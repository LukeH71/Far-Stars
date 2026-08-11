
#import "Renderer.h"
#import "Fonts.h"
#import "Shaders.h"
#import "MathUtilities.h"
#import "PhysicsEntity.h"
#import "PlayerController.h"
#import "UI.h"

#import "KeyBinds.h"
#import "KeyboardMTKView.h"

#import <vector>
#import <chrono>

#import <iostream>

/// Aligns a value to an address.
static size_t Align (size_t value, size_t align)
{
    if (align == 0)
    {
        return value;
    }
    else if ((value & (align-1)) == 0)
    {
        return value;
    }
    else
    {
        return (value+align) & ~(align-1);
    }
}

@implementation Renderer
{
    dispatch_semaphore_t _inFlightSemaphore;
    id<MTLCommandQueue> _commandQueue;
    
    PlayerController *_player;
    UIController *_UIController;

    id<MTLDepthStencilState> _lessEqualDepthStencilState;
    id<MTLDepthStencilState> _noWriteLessEqualDepthStencilState;
    id<MTLDepthStencilState> _noDepthStencilState;

    id<MTLRenderPipelineState> _opaquePipeline;
    id<MTLRenderPipelineState> _initImageBlockPipeline;
    id<MTLRenderPipelineState> _transparencyPipeline;
    id<MTLRenderPipelineState> _blendPipelineState;
    id<MTLRenderPipelineState> _GUIPipelineState;
    id<MTLRenderPipelineState> _textPipelineState;
    id<MTLRenderPipelineState> _cursorPipelineState;
    id<MTLRenderPipelineState> _highlightTextPipelineState;
    
    
    id<MTLComputePipelineState> _textCompute;
    
    MTLRenderPassDescriptor* _forwardRenderPassDescriptor;

    id<MTLBuffer> _actorParamsBuffers   [MaxBuffersInFlight];
    id<MTLBuffer> _cameraParamsBuffers  [MaxBuffersInFlight];

    id<MTLBuffer> _actorMesh;

    std::vector<PhysicsEntity> _opaqueActors;
    std::vector<PhysicsEntity> _transparentActors;

    MTLSize _tileSize;
    matrix_float4x4 _projectionMatrix;
    float _rotation;
    int8_t _currentBufferIndex;
    
    
    std::unordered_map<std::string, void*> _debugValues;
    std::chrono::microseconds _frameDuration;
    std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<long long, std::ratio<1LL, 1000000000LL>>> _timeSinceLastFrame;
    float _deltaTime;
    
}

- (nonnull instancetype) initWithMetalKitView:(KeyboardMTKView *)mtkView
{
    self = [super init];
    if (self)
    {
        _device = mtkView.device;
        _mtkView = mtkView;
        [self loadResources];
        [self loadMetal];
        _UIController = new UIController(_device, mtkView, _player, _commandQueue, _GUIPipelineState, _textPipelineState, _cursorPipelineState, _highlightTextPipelineState, _textCompute, &_debugValues);
        //_UIController->loadTextSize(100, _textCompute, _textCompute, _commandQueue);
        //_UIController->stepSpacing(0);
        [mtkView setGetText: ^(){
            std::string data = self->_UIController->getCopiedText();
            char* newData = new char[data.length()+1]; // own lifetime
            std::strcpy(newData, data.c_str());
            return newData;
        }];
        
        
        
        
        
    }
    
    
    
    return self;
}



/// Initializes the app's starting values, and creates actors and constant buffers.
- (void) loadResources
{
    
    uint8_t enabledKeys[KEY_LENGTH] = DEFAULT_KEY_BINDS;
    
    [_mtkView initKeys: enabledKeys];
    
    _player = new PlayerController(_mtkView, _projectionMatrix);
    
    _debugValues["pos"] = (void*)_player->getPos();
    //_UIController = new UIController(_device, _commandQueue);
    
    _inFlightSemaphore = dispatch_semaphore_create(MaxBuffersInFlight);
    _currentBufferIndex = 0;
    _enableOrderIndependentTransparency = NO;
    _rotation = 0.0f;
    _enableRotation = true;

    // This variable is necissary for using an image block. Smaller values yeild in worse preformance,
    // but more resolution. Larger values yeild in better preformance because the image block takes up
    // less space, but also will result in worse preformance.
    _tileSize = MTLSizeMake(32lu, 16lu, 1lu);

    // Create the floor
    {
        vector_float4 color = vector4(.01f, .01f, .01f, 1.f);
        PhysicsEntity actor = PhysicsEntity(color, (vector_float3){0.f, -2.f, 6.f}, (vector_float3){8.f, 1.f, 9.f}, (vector_float3){0.f, 0.f, 0.f}, 0);
        _opaqueActors.push_back(actor);
    }

    // Create rotating cube
    {
        PhysicsEntity actor = PhysicsEntity(vector4(0.1f, 0.4f, 1.0f, 0.01f), vector3(0.f, 0.6f, 0.f), vector3(1.5f, 1.5f, 1.5f), vector3(90.f, 0.f, 0.f), 0);
        _transparentActors.push_back(actor);
    }
        

    // Create the constant buffers for each frame.
    for (NSUInteger i = 0; i < MaxBuffersInFlight; ++i)
    {
        id<MTLBuffer> actorParamsBuffer = [_device newBufferWithLength: Align(sizeof(ActorParams), BufferOffsetAlign) * 2 options:MTLResourceStorageModeShared];
        actorParamsBuffer.label = [NSString stringWithFormat:@"actor params[%lu]", i];
        _actorParamsBuffers[i] = actorParamsBuffer;

        id<MTLBuffer> cameraParamsBuffer = [_device newBufferWithLength:sizeof(CameraParams) options:MTLResourceStorageModeShared];
        cameraParamsBuffer.label = [NSString stringWithFormat:@"camera params[%lu]", i];
        _cameraParamsBuffers[i] = cameraParamsBuffer;
    }
}

/// Creates the Metal render state objects.
- (void) loadMetal
{
    // Check that this GPU supports raster order groups.
    _supportsOrderIndependentTransparency = [_device supportsFamily:MTLGPUFamilyApple4];

    NSError* error;

    //NSLog(@"Selected Device: %@", _mtkView.device.name);

    _mtkView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _mtkView.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;

    _commandQueue = [_device newCommandQueue];
    id<MTLLibrary> defaultLibrary = [_device newDefaultLibrary];
    {
        
        id<MTLFunction> vertexFunc = [defaultLibrary newFunctionWithName:@"forwardVertex"];
        id<MTLFunction> fragmentFunc = [defaultLibrary newFunctionWithName:@"processOpaqueFragment"];

        MTLRenderPipelineDescriptor* renderPipelineDesc = [MTLRenderPipelineDescriptor new];
        renderPipelineDesc.label = @"Unordered alpha blending pipeline";
        renderPipelineDesc.vertexFunction = vertexFunc;
        renderPipelineDesc.fragmentFunction = fragmentFunc;
        renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].pixelFormat = _mtkView.colorPixelFormat;
        renderPipelineDesc.depthAttachmentPixelFormat = _mtkView.depthStencilPixelFormat;
        renderPipelineDesc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;

        renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].blendingEnabled = true;
        renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].alphaBlendOperation = MTLBlendOperationAdd;
        renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].sourceAlphaBlendFactor = MTLBlendFactorOne;
        renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].destinationAlphaBlendFactor = MTLBlendFactorZero;
        renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].rgbBlendOperation = MTLBlendOperationAdd;
        renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].writeMask = MTLColorWriteMaskAll;

        _opaquePipeline = [_device newRenderPipelineStateWithDescriptor:renderPipelineDesc
                                                                  error:&error];
        NSAssert(_opaquePipeline, @"Failed to create opaque render pipeline state: %@", error);
    }

    // Only use transparency effects if the device supports tiles shaders and image blocks.
    if (_supportsOrderIndependentTransparency)
    {
        _enableOrderIndependentTransparency = YES;

        // Set up the transparency pipeline so that it populates the image block with fragment values.
        {
            id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"forwardVertex"];
            id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"processTransparentFragment"];

            MTLRenderPipelineDescriptor* renderPipelineDesc = [MTLRenderPipelineDescriptor new];
            renderPipelineDesc.label = @"Transparent Fragment Store Op";
            renderPipelineDesc.vertexFunction = vertexFunction;
            renderPipelineDesc.fragmentFunction = fragmentFunction;
            renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].blendingEnabled = NO;

            // Disable the color write mask.
            // This fragment shader only writes color data into the image block.
            // It doesn't produce an output for the color attachment.
            renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].writeMask = MTLColorWriteMaskNone;
            renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].pixelFormat = _mtkView.colorPixelFormat;
            renderPipelineDesc.depthAttachmentPixelFormat = _mtkView.depthStencilPixelFormat;
            renderPipelineDesc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;

            _transparencyPipeline = [_device newRenderPipelineStateWithDescriptor:renderPipelineDesc error:&error];
            NSAssert(_transparencyPipeline, @"Failed to create transparency render pipeline state: %@", error);
        }
        // Configure the kernel tile shader to initialize the image block for each frame.
        {
            id<MTLFunction> kernelTileFunction = [defaultLibrary newFunctionWithName:@"initTransparentFragmentStore"];
            MTLTileRenderPipelineDescriptor *tileDesc = [MTLTileRenderPipelineDescriptor new];
            tileDesc.label = @"Init Image Block Kernel";
            tileDesc.tileFunction = kernelTileFunction;
            tileDesc.colorAttachments[AAPLRenderTargetColor].pixelFormat = _mtkView.colorPixelFormat;
            tileDesc.threadgroupSizeMatchesTileSize = YES;

            _initImageBlockPipeline = [_device newRenderPipelineStateWithTileDescriptor:tileDesc options:0 reflection:nil error:&error];
            NSAssert(_initImageBlockPipeline, @"Failed to create init image block tile shaer pipeline: %@", error);
        }
        // Configure the pipeline to blend transparent and opaque fragments.
        {
            id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"quadPassVertex"];
            id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"blendFragments"];
            MTLRenderPipelineDescriptor* renderPipelineDesc = [MTLRenderPipelineDescriptor new];
            renderPipelineDesc.label = @ "Transparent Fragment Blending";
            renderPipelineDesc.vertexFunction = vertexFunction;
            renderPipelineDesc.fragmentFunction = fragmentFunction;
            renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].pixelFormat = _mtkView.colorPixelFormat;
            renderPipelineDesc.depthAttachmentPixelFormat = _mtkView.depthStencilPixelFormat;
            renderPipelineDesc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
            renderPipelineDesc.vertexDescriptor = nil;

            _blendPipelineState = [_device newRenderPipelineStateWithDescriptor:renderPipelineDesc error:&error];
            NSAssert(_blendPipelineState, @"Failed to create blend pipeline state: %@", error);
        }
        // Set up the GUI pipeline
        {
            MTLVertexDescriptor *mtlVertexDescriptor = [[MTLVertexDescriptor alloc] init];

            id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"flatVertexShader"];
            id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"colorMapFragmentShader"];

            MTLRenderPipelineDescriptor* renderPipelineDesc = [MTLRenderPipelineDescriptor new];
            renderPipelineDesc.label = @"GUI pipeline";
            renderPipelineDesc.vertexFunction = vertexFunction;
            renderPipelineDesc.fragmentFunction = fragmentFunction;
            renderPipelineDesc.vertexDescriptor = mtlVertexDescriptor;
            renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].pixelFormat = _mtkView.colorPixelFormat;
            renderPipelineDesc.depthAttachmentPixelFormat = _mtkView.depthStencilPixelFormat;
            renderPipelineDesc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
            //renderPipelineDesc.vertexDescriptor = nil;

            _GUIPipelineState = [_device newRenderPipelineStateWithDescriptor:renderPipelineDesc error:&error];
            NSAssert(_GUIPipelineState, @"Failed to create blend pipeline state: %@", error);
        }
        
        // Set up the text pipeline
        {
            MTLVertexDescriptor *mtlVertexDescriptor = [[MTLVertexDescriptor alloc] init];

            id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"textVertex"];
            id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"colorTextFragment"];

            MTLRenderPipelineDescriptor* renderPipelineDesc = [MTLRenderPipelineDescriptor new];
            renderPipelineDesc.label = @"r pipeline";
            renderPipelineDesc.vertexFunction = vertexFunction;
            renderPipelineDesc.fragmentFunction = fragmentFunction;
            renderPipelineDesc.vertexDescriptor = mtlVertexDescriptor;
            renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].pixelFormat = _mtkView.colorPixelFormat;
            renderPipelineDesc.depthAttachmentPixelFormat = _mtkView.depthStencilPixelFormat;
            renderPipelineDesc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
            //renderPipelineDesc.vertexDescriptor = nil;

            _textPipelineState = [_device newRenderPipelineStateWithDescriptor:renderPipelineDesc error:&error];
            NSAssert(_GUIPipelineState, @"Failed to create blend pipeline state: %@", error);
        }//id<MTLComputePipelineState> _textCompute;
        
        {
            MTLVertexDescriptor *mtlVertexDescriptor = [[MTLVertexDescriptor alloc] init];

            id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"cursorVertex"];
            id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"cursorFragment"];

            MTLRenderPipelineDescriptor* renderPipelineDesc = [MTLRenderPipelineDescriptor new];
            renderPipelineDesc.label = @"cursor";
            renderPipelineDesc.vertexFunction = vertexFunction;
            renderPipelineDesc.fragmentFunction = fragmentFunction;
            renderPipelineDesc.vertexDescriptor = mtlVertexDescriptor;
            renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].pixelFormat = _mtkView.colorPixelFormat;
            renderPipelineDesc.depthAttachmentPixelFormat = _mtkView.depthStencilPixelFormat;
            renderPipelineDesc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
            //renderPipelineDesc.vertexDescriptor = nil;

            _cursorPipelineState = [_device newRenderPipelineStateWithDescriptor:renderPipelineDesc error:&error];
            NSAssert(_cursorPipelineState, @"Failed to create blend pipeline state: %@", error);
        }//id<MTLComputePipelineState> _textCompute;
        
        {
            MTLVertexDescriptor *mtlVertexDescriptor = [[MTLVertexDescriptor alloc] init];

            id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"simpleVert"];
            id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"invertFragment"];

            MTLRenderPipelineDescriptor* renderPipelineDesc = [MTLRenderPipelineDescriptor new];
            renderPipelineDesc.label = @"highlight";
            renderPipelineDesc.vertexFunction = vertexFunction;
            renderPipelineDesc.fragmentFunction = fragmentFunction;
            renderPipelineDesc.vertexDescriptor = mtlVertexDescriptor;
            renderPipelineDesc.colorAttachments[AAPLRenderTargetColor].pixelFormat = _mtkView.colorPixelFormat;
            renderPipelineDesc.depthAttachmentPixelFormat = _mtkView.depthStencilPixelFormat;
            renderPipelineDesc.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
            
            //renderPipelineDesc.vertexDescriptor = nil;

            _highlightTextPipelineState = [_device newRenderPipelineStateWithDescriptor:renderPipelineDesc error:&error];
            NSAssert(_cursorPipelineState, @"Failed to create blend pipeline state: %@", error);
        }//id<MTLComputePipelineState> _textCompute;
        
        
        
        
        
        // Set up the text pipeline
        {
            //MTLVertexDescriptor *mtlVertexDescriptor = [[MTLVertexDescriptor alloc] init];

            id<MTLFunction> shadeFunction = [defaultLibrary newFunctionWithName:@"colorTextKernel"];


            _textCompute = [_device newComputePipelineStateWithFunction:shadeFunction error:&error];
            NSAssert(_textCompute, @"Failed to create blend pipeline state: %@", error);
        }
        
        
        
    }
    else
    {
        _enableOrderIndependentTransparency = NO;
    }

    {
        MTLDepthStencilDescriptor* depthStencilDesc = [MTLDepthStencilDescriptor new];
        depthStencilDesc.label = @"DepthCompareAlwaysAndNoWrite";
        depthStencilDesc.depthWriteEnabled = FALSE;
        depthStencilDesc.depthCompareFunction = MTLCompareFunctionAlways;
        depthStencilDesc.backFaceStencil = nil;
        depthStencilDesc.frontFaceStencil = nil;
        _noDepthStencilState = [_device newDepthStencilStateWithDescriptor:depthStencilDesc];

        depthStencilDesc.label = @"DepthCompareLessEqualAndWrite";
        depthStencilDesc.depthWriteEnabled = TRUE;
        depthStencilDesc.depthCompareFunction = MTLCompareFunctionLessEqual;
        _lessEqualDepthStencilState = [_device newDepthStencilStateWithDescriptor:depthStencilDesc];

        depthStencilDesc.label = @"DepthCompareLessEqualAndNoWrite";
        depthStencilDesc.depthWriteEnabled = FALSE;
        _noWriteLessEqualDepthStencilState = [_device newDepthStencilStateWithDescriptor:depthStencilDesc];
    }

    _forwardRenderPassDescriptor = [MTLRenderPassDescriptor new];
    _forwardRenderPassDescriptor.colorAttachments[AAPLRenderTargetColor].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    _forwardRenderPassDescriptor.colorAttachments[AAPLRenderTargetColor].loadAction = MTLLoadActionClear;
    _forwardRenderPassDescriptor.colorAttachments[AAPLRenderTargetColor].storeAction = MTLStoreActionStore;
    _forwardRenderPassDescriptor.depthAttachment.clearDepth = 1.0;
    _forwardRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
    _forwardRenderPassDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;

    if (_supportsOrderIndependentTransparency)
    {
        // Set the tile size for the fragment shader.
        _forwardRenderPassDescriptor.tileWidth  = _tileSize.width;
        _forwardRenderPassDescriptor.tileHeight = _tileSize.height;

        // Set the image block's memory size.
        _forwardRenderPassDescriptor.imageblockSampleLength = _transparencyPipeline.imageblockSampleLength;
    }

    {
        const Vertex quadVertices[] =
        {
            { {  1,  1, -1 } }, // Front
            { { -1,  1, -1 } },
            { { -1,  1,  1 } },

            { {  1,  1, -1 } },
            { { -1,  1,  1 } },
            { {  1,  1,  1 } },
            
            { {  1,  -1, -1 } }, // Back
            { { -1,  -1, -1 } },
            { { -1,  -1,  1 } },

            { {  1,  -1, -1 } },
            { { -1,  -1,  1 } },
            { {  1,  -1,  1 } },
            
            { {  -1,  1, -1 } }, // Left
            { { -1,  -1, -1 } },
            { { -1,  -1,  1 } },

            { {  -1,  1, -1 } },
            { { -1,  -1,  1 } },
            { {  -1,  1,  1 } },
            
            { {  1,  1, -1 } }, // Right
            { { 1,  -1, -1 } },
            { { 1,  -1,  1 } },

            { { 1,  1, -1 } },
            { { 1,  -1,  1 } },
            { { 1,  1,  1 } },
            
            { {  1,  -1, 1 } }, // Down
            { { -1,  -1, 1 } },
            { { -1,  1,  1 } },

            { {  1,  -1, 1 } },
            { { -1,  1,  1 } },
            { {  1,  1,  1 } },
            
            { {  1,  -1, -1 } }, // up
            { { -1,  -1, -1 } },
            { { -1,  1,  -1 } },

            { {  1,  -1, -1 } },
            { { -1,  1,  -1 } },
            { {  1,  1,  -1 } },
        };

        _actorMesh = [_device newBufferWithBytes:quadVertices
                                          length:sizeof(quadVertices)
                                         options:MTLResourceStorageModeShared];
        _actorMesh.label = @"Quad Mesh";

        NSAssert(_actorMesh, @"Error creating actor vertex buffer: %@", error);
    }
}


/// Delegate callback that responds to changes in the device's orientation or view size changes.
- (void) mtkView:(nonnull MTKView*) view drawableSizeWillChange:(CGSize)size
{
    
    //std::cout << _mtkView.window.frame.size.height << std::endl;
    
    NSRect trackingRect = [view bounds];
    NSTrackingAreaOptions options = NSTrackingMouseMoved | NSTrackingActiveInKeyWindow;
    NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:trackingRect options:options owner:view userInfo:nil];
    
    // Add the tracking area to the view
    [view addTrackingArea:trackingArea];
    
    float aspect = (float)size.width / (float)size.height;
    _projectionMatrix = matrix_perspective_left_hand(radians_from_degrees(65.f), aspect, 1.f, 150.f);
    _player->setProjectionMatrix(_projectionMatrix);
    if (_mtkView.paused)
    {
        [_mtkView draw];
    }
    
    _UIController->setAspect(aspect);
    _UIController->recalculateCharacterAtlas(int(size.height));
}

/// Updates the application's state for the current frame.
- (void) updateState
{
    
    
    _currentBufferIndex = ((_currentBufferIndex + 1) % MaxBuffersInFlight);

    ActorParams* actorParams = (ActorParams*) _actorParamsBuffers[_currentBufferIndex].contents;

    for (NSUInteger i = 0; i < _opaqueActors.size(); ++i)
    {
        matrix_float4x4 translationMatrix = matrix4x4_translation(_opaqueActors[i].getPosition());
        matrix_float4x4 scaleMatrix = matrix4x4_scale(_opaqueActors[i].getScale());
        matrix_float4x4 rotationXMatrix = matrix4x4_rotation(radians_from_degrees(_opaqueActors[i].getRotation().x), 1.f, 0.f, 0.f);
        matrix_float4x4 rotationMatrix = matrix_multiply(matrix4x4_rotation(radians_from_degrees(_rotation), 0.f, 1.f, 0.f), rotationXMatrix);

        // Last opaque actor is tbe floor which has no rotation.
        if (i == _opaqueActors.size() - 1)
        {
            rotationMatrix = matrix_identity_float4x4;
        }
        actorParams[i].modelMatrix = matrix_multiply(translationMatrix, matrix_multiply(rotationMatrix, scaleMatrix));
        actorParams[i].color = _opaqueActors[i].getColor();
    }

    for (NSUInteger i = 0; i < _transparentActors.size(); ++i)
    {
        NSUInteger paramsIndex = i + _opaqueActors.size();

        matrix_float4x4 translationMatrix = matrix4x4_translation(_transparentActors[i].getPosition());
        matrix_float4x4 scaleMatrix = matrix4x4_scale(_transparentActors[i].getScale());
        matrix_float4x4 rotationXMatrix = matrix4x4_rotation(radians_from_degrees(_transparentActors[i].getRotation().x), 1.f, 0.f, 0.f);
        matrix_float4x4 rotationMatrix = matrix_multiply(matrix4x4_rotation(radians_from_degrees(_rotation), 0.f, 1.f, 0.f), rotationXMatrix);

        actorParams[paramsIndex].modelMatrix = matrix_multiply(translationMatrix, matrix_multiply(rotationMatrix, scaleMatrix));
        actorParams[paramsIndex].color = _transparentActors[i].getColor();
    }

    //CameraParams* cameraParams = (CameraParams*) _cameraParamsBuffers[_currentBufferIndex].contents;
    _player->tickController(((CameraParams*) _cameraParamsBuffers[_currentBufferIndex].contents));
    
    
    
    _UIController->GUILogic(_currentBufferIndex);
    //_UIController->loadTextSize(_mtkView, 5.0, _textCompute, _commandQueue);
    
    if (_enableRotation)
    {
        _rotation += 1.f;
    }
}

// Draws all opaque meshes from the opaque actors array.
- (void) drawOpaqueObjects:(id<MTLRenderCommandEncoder>) renderEncoder
            renderPipeline:(id<MTLRenderPipelineState>) renderPipelineState
{
    [renderEncoder pushDebugGroup:@"Opaque Actor Rendering"];

    [renderEncoder setRenderPipelineState:renderPipelineState];
    [renderEncoder setCullMode:MTLCullModeNone];
    [renderEncoder setDepthStencilState:_lessEqualDepthStencilState];

    for (NSUInteger actorIndex = 0; actorIndex < _opaqueActors.size(); ++actorIndex)
    {
        size_t offsetValue = actorIndex * Align(sizeof(ActorParams), BufferOffsetAlign);

        [renderEncoder setVertexBufferOffset:offsetValue atIndex:AAPLBufferIndexActorParams];
        [renderEncoder setFragmentBufferOffset:offsetValue atIndex:AAPLBufferIndexActorParams];

        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                          vertexStart:0
                          vertexCount:6];
    }
    [renderEncoder popDebugGroup];
}

// Draws all the transparent meshes from the transparent actors array.
- (void) drawTransparentObjects:(id<MTLRenderCommandEncoder>) renderEncoder
                 renderPipeline:(id<MTLRenderPipelineState>) renderPipelineState
{
    [renderEncoder pushDebugGroup:@"Transparent Actor Rendering"];

    [renderEncoder setRenderPipelineState:renderPipelineState];
    [renderEncoder setCullMode:MTLCullModeNone];

    // Only test the depth of the transparent geometry against the opaque geometry. This allows
    // transparent fragments behind other transparent fragments to be rasterized and stored in
    // the image block structure.
    [renderEncoder setDepthStencilState:_noWriteLessEqualDepthStencilState];

    for (NSUInteger actorIndex = 0; actorIndex < _transparentActors.size(); ++actorIndex)
    {
        size_t offsetValue = (actorIndex + _opaqueActors.size()) * Align(sizeof(ActorParams), BufferOffsetAlign);

        [renderEncoder setVertexBufferOffset:offsetValue atIndex:AAPLBufferIndexActorParams];
        [renderEncoder setFragmentBufferOffset:offsetValue atIndex:AAPLBufferIndexActorParams];

        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                          vertexStart:0
                          vertexCount:36];
    }
    [renderEncoder popDebugGroup];
}


// Binds the constant buffer the app needs to render the 3D actors.
- (void) bindCommonActorBuffers:(id<MTLRenderCommandEncoder>) renderEncoder;
{
    [renderEncoder pushDebugGroup:@"Common Buffer Binding"];

    [renderEncoder setVertexBuffer:_actorMesh offset:0 atIndex:AAPLBufferIndexVertices];

    [renderEncoder setVertexBuffer:_cameraParamsBuffers[_currentBufferIndex] offset:0 atIndex:AAPLBufferIndexCameraParams];

    [renderEncoder setVertexBuffer:_actorParamsBuffers[_currentBufferIndex] offset:0 atIndex:AAPLBufferIndexActorParams];

    [renderEncoder setFragmentBuffer:_actorParamsBuffers[_currentBufferIndex] offset:0 atIndex:AAPLBufferIndexActorParams];

    [renderEncoder popDebugGroup];
}

// Creates a new command buffer after waiting for the last frame to complete.
- (id<MTLCommandBuffer>) beginFrame
{
    dispatch_semaphore_wait(_inFlightSemaphore, DISPATCH_TIME_FOREVER);

    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    commandBuffer.label = @"Drawable Command Buffer";

    __block dispatch_semaphore_t blockSemaphore = _inFlightSemaphore;
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer)
     {
        dispatch_semaphore_signal(blockSemaphore);
    }];

    [self updateState];
    
    
    return commandBuffer;
}

// Draws the opaque and transparent meshes with an explicit image block in a fragment function that implements order-independent transparency.
- (void) drawWithOrderIndependentTransparency:(nonnull MTKView*)view commandBuffer:(id<MTLCommandBuffer>) commandBuffer
{
    // Drawable lets you use [[color(0)]]
    id<MTLTexture> drawable = _mtkView.currentDrawable.texture;

    // Error handling
    if (!drawable)
        return;

    _forwardRenderPassDescriptor.colorAttachments[AAPLRenderTargetColor].texture = drawable;
    _forwardRenderPassDescriptor.depthAttachment.texture = _mtkView.depthStencilTexture;

    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:_forwardRenderPassDescriptor];
    renderEncoder.label = @"Forward Pass";

    // Initialize the image block's memory before rendering.
    [renderEncoder pushDebugGroup:@"Init Image Block"];
    [renderEncoder setRenderPipelineState:_initImageBlockPipeline];
    [renderEncoder dispatchThreadsPerTile:_tileSize];
    [renderEncoder popDebugGroup];

    // Render opaque and transparent actors
    [self bindCommonActorBuffers:renderEncoder];
    [self drawOpaqueObjects:renderEncoder renderPipeline:_opaquePipeline];
    [self drawTransparentObjects:renderEncoder renderPipeline:_transparencyPipeline];
    
    // Blend opaque and transparent actors (make them both visible)
    [renderEncoder pushDebugGroup:@"Blend Fragments"];
    [renderEncoder setRenderPipelineState:_blendPipelineState];
    [renderEncoder setCullMode:MTLCullModeNone];
    [renderEncoder setDepthStencilState:_noDepthStencilState];
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
    [renderEncoder popDebugGroup];
    
    // Draw UI
    [renderEncoder setDepthStencilState:_noDepthStencilState];
    [renderEncoder setRenderPipelineState:_GUIPipelineState];
    [renderEncoder setCullMode:MTLCullModeNone];
    
    _UIController->drawUI(renderEncoder, _currentBufferIndex);

    
    // Finish the use of this render encoder
    [renderEncoder endEncoding];
}

/// Draws the opaque and transparent meshes with a pipeline's alpha blending.
- (void) drawUnorderedAlphaBlending:(nonnull MTKView*) view
                      commandBuffer:(id<MTLCommandBuffer>) commandBuffer
{
    id<MTLTexture> drawable = _mtkView.currentDrawable.texture;

    if (!drawable)
    {
        return;
    }

    _forwardRenderPassDescriptor.colorAttachments[AAPLRenderTargetColor].texture = drawable;
    _forwardRenderPassDescriptor.depthAttachment.texture = _mtkView.depthStencilTexture;

    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:_forwardRenderPassDescriptor];
    renderEncoder.label = @"Forward Render Pass";

    [self bindCommonActorBuffers:renderEncoder];
    [self drawOpaqueObjects:renderEncoder renderPipeline:_opaquePipeline];
    [self drawTransparentObjects:renderEncoder renderPipeline:_opaquePipeline];

    [renderEncoder endEncoding];
}

/// Renders a single frame to a MetalKit view.
- (void) drawInMTKView:(nonnull MTKView*) view
{
    
    auto start = std::chrono::high_resolution_clock::now();

    
    
    id<MTLCommandBuffer> commandBuffer = [self beginFrame];
    if (_enableOrderIndependentTransparency)
    {
        //[self recalculateAspectRatioWithOldAspect: 1.333 newAspect: 1.334];
        [self drawWithOrderIndependentTransparency:view commandBuffer:commandBuffer];
    }
    else
    {
        [self drawUnorderedAlphaBlending:view commandBuffer:commandBuffer];
    }
    // Drawable lets you use [[color(0)]]
    id<MTLTexture> drawable = _mtkView.currentDrawable.texture;

    // Error handling
    if (!drawable)
        return;

  //  _forwardRenderPassDescriptor.colorAttachments[AAPLRenderTargetColor].texture = drawable;
//    _forwardRenderPassDescriptor.depthAttachment.texture = _mtkView.depthStencilTexture;
//
//    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:_forwardRenderPassDescriptor];
//    renderEncoder.label = @"Forward Pass";
    
    
    

    [commandBuffer presentDrawable:_mtkView.currentDrawable];
    [commandBuffer commit];
    
    auto end = std::chrono::high_resolution_clock::now();

    _frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    
    
    _deltaTime = float(std::chrono::duration_cast<std::chrono::microseconds>(end - _timeSinceLastFrame).count());
    
    //std::cout << _deltaTime << "\n";
    
    
    _debugValues["time"] = (void*)(&_frameDuration);
    _debugValues["fps"] = (void*)(&_deltaTime);
    
    
    _timeSinceLastFrame = std::chrono::high_resolution_clock::now();
}
@end


// ms/f -> f/s
