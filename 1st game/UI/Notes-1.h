/*
 Notes (1):
 
    A test of how a font works as one block of text, as well as a notebook. Also, a test on image rendering in UI
 
  
  - Static:
     - Image background

  - Text:
     - Static text ("I am a static text group!")
     - Variable text (user-defined)

*/


void UIController::loadNotes(){
    
    
    _numElementsStatic = 1; //2
    _numElementsButtons = _numElementsStatic; // 0
    _numElementsDraggable = _numElementsButtons; // 0
    _numElementsTextFeilds = _numElementsDraggable + 2; // 2
    
//    textPerameters params[] = PARAMS_READ;
//    
//    _textPerameterBuffer = [_device newBufferWithBytes:&params
//                                                   length:sizeof(params)
//                                                  options:MTLResourceStorageModeShared];

    
    
    static const GUIVertex UIMesh[] = {
        
        { { -1.6, -1 }, 0, 0 },
        { { -1.6, -0.7 }, 1, 0 }, // bottom (1)
        { { -1.6, 0 }, 1, 0 }, // top (2)
        
    };
    
    static const GUIVertexUniforms UIUniforms[] = {
        { {3.2, 2} },
        { {3.2, 1} },
    };
    
    static const GUITextureUniforms UITexUniforms[] = {
        { {0/160.f, 100/100.f}, {160/160.f, -100/100.f} },
    };
    
    //_values.push_back(30);
    

        
    _stateXElementSizes = [_device newBufferWithBytes:UIUniforms
                                                      length:sizeof(UIUniforms)
                                                     options:MTLResourceStorageModeShared];
    
    _stateXElementTextures = [_device newBufferWithBytes:UITexUniforms
                                                         length:sizeof(UITexUniforms)
                                                        options:MTLResourceStorageModeShared];
    
    _stateXElements = [_device newBufferWithBytes:UIMesh
                                                         length:sizeof(UIMesh)
                                                        options:MTLResourceStorageModeShared];
    
    
    //NSError *error;
    
    MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:_device];
    
    NSDictionary *textureLoaderOptions =
    @{
        MTKTextureLoaderOptionTextureUsage       : @(MTLTextureUsageShaderRead),
        MTKTextureLoaderOptionTextureStorageMode : @(MTLStorageModePrivate)
    };
    
    _stateXTextureMap = [textureLoader newTextureWithName:@"Notes-1"
                                              scaleFactor:1.0
                                                   bundle:nil
                                                  options:textureLoaderOptions
                                                    error:nullptr];
    
    
    std::vector<std::string> textGroupsStatic = {"I am a static text group!"};
    std::vector<std::string> textGroupsChangeable = {"Edit me :D"};
    
    textPerameters paramsStatic = {MAX_LENGTH_LINES_FLAG, simd_make_float3(0,0.25,0.25), 3, -1.5, 0.75, 0.2, 0 ,0, 1};
    textPerameters paramsChangeable = {MAX_LENGTH_LINES_FLAG, simd_make_float3(0,0,1), 3, -1.5, 0, 0.2, 0 ,0, 2};
    
    _stateXTextGroups = addTextGroups(textGroupsStatic, textGroupsChangeable, {paramsStatic, paramsChangeable});

    
    recalculateCharacterAtlas(int(_mtkView.drawableSize.height));
}



void UIController::switchUIStateNotes(){
    if ([_mtkView getKey:OPEN_NOTES_DEMO]){
        if(_state!=0){
            _state = 0;
            [_mtkView setInTextFeild:false];
            _playerController->unlockCameraLockMouse();
            popTextGroups(_stateXTextGroups);
        } else {
            _state = 1;
            loadNotes();
            [_mtkView setInTextFeild:true];
            _playerController->lockCameraUnlockMouse();
            
            
            //stepSpacing(0);
        }
        [_mtkView clearKey:OPEN_NOTES_DEMO];
    }
}



void UIController::stepUILogicNotes(uint8_t currentBufferIndex){
    if([_mtkView getKey:6]) { // mouse click
        CGPoint mousePos = [_mtkView getMousePos];
        
        mousePos.x /= ((_aspect < 1.6f) ? 1/1.6 : (1/_aspect));
        mousePos.y /= ((_aspect > 1.6f) ? 1 : _aspect/1.6);
        
        checkIfInBounds(mousePos);
        
        clickTextFeild(mousePos);
        
        
        
        
    } else {
        _mouseClickCkeck = false;
        //_highlightStartIndex = 0;
        _selectedElement = 255; // invalid
    }
    
    
    
    
}


// Draws all GUI related
void UIController::drawNotes (id<MTLRenderCommandEncoder> renderEncoder, uint8_t currentBufferIndex)
{
    [renderEncoder pushDebugGroup:@"Font Designer UI"];
    
    [renderEncoder setVertexBuffer:_stateXElements
                            offset:0
                           atIndex:0];
    
    [renderEncoder setVertexBuffer:_stateXElementSizes
                            offset:0
                           atIndex:1];
    
    [renderEncoder setVertexBytes:&_aspect
                            length:sizeof(_aspect)
                           atIndex:2];
    
    [renderEncoder setVertexBuffer:_stateXElementTextures
                            offset:0
                           atIndex:3];
    
    [renderEncoder setVertexBytes:_scrollOffset
                            length:sizeof(float)
                           atIndex:4];
    
    [renderEncoder setFragmentTexture:_stateXTextureMap atIndex:0];
    

    [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                              indexCount:6
                               indexType:MTLIndexTypeUInt16
                             indexBuffer:_squareIndexBuffer
                       indexBufferOffset:0
                           instanceCount:_numElementsDraggable];
    
    [renderEncoder popDebugGroup];
}
