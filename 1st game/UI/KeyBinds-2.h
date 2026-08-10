/*
 KeyBinds (2):
 
    A way to edit keybinds.
 
 
    Scrollable
 
  
  - Static:
     - Red background
     - Black semi-transparent background
     - Bar for visability of names of each key bind
     - Title bar
 
  - Buttons:
     - A button for each key bind. Once clicked, one can set the new value of a keybind.


  - Text:
     - A title that says "Key Binds Editor"
 
     - For each keybind:
       - A key bind title (ex. Move Forward)
       - A kay bind value (ex. Up Arrow)

*/



void UIController::loadKeyBinds(){
    
    _numElementsStatic = 4; //2
    _numElementsButtons = _numElementsStatic; // 0
    _numElementsDraggable = _numElementsButtons + 0; // 0
    _numElementsTextFeilds = _numElementsDraggable; // 0

    
//    textPerameters params[] = PARAMS_READ;
//
//    _textPerameterBuffer = [_device newBufferWithBytes:&params
//                                                   length:sizeof(params)
//                                                  options:MTLResourceStorageModeShared];

    // texture: 102x25
     
    
    std::vector<GUIVertex> elements = {
        
        { { -5, -5 }, 0, 2 },
        { { -5, -5 }, 0, 0 },
        { { -1.5, -5 }, 1, 1 },
        { { -0.3, 0.65 }, 2, 1 },
        //{ { -1.6, -0.7 }, 1, 0 }, // bottom (1)
        
    };
    
    static const GUIVertexUniforms elementSizes[] = {
        { {10, 10} },
        { {1, 10} },
        { {1.7, 0.3} },
        { {1, 0.175} },
    };
    
    static const GUITextureUniforms elementTextures[] = {
        { {0/102.f, 4/25.f}, {1/102.f, 1/25.f} },
        { {0/102.f, 3/25.f}, {1/102.f, 1/25.f} },
        { {0/102.f, 24/25.f}, {1/102.f, 1/25.f} },
        { {0/102.f, 5/25.f}, {1/102.f, 1/25.f} },
        { {0/102.f, 1/25.f}, {1/102.f, 1/25.f} },
    };
    
    //_values.push_back(30);
    
    std::vector<std::string> textGroupsStatic = {"Key Binds Editor"};
    
    textPerameters baseKeyBindParam = {MAX_LENGTH_LINES_FLAG, simd_make_float3(1,1,1), 3, -1.4, 0.4, 0.1, 0 ,0, 20};
    textPerameters baseKeyBindInputParam = {MAX_LENGTH_LINES_FLAG, simd_make_float3(1,1,1), 3, -0.25, 0.4, 0.1, 0 ,0, 20};
    
    std::vector<textPerameters> titleParams = {{MAX_LENGTH_LINES_FLAG, simd_make_float3(1,1,1), 3, -0.2, 0.7, 0.2, 0 ,0, 20}};
    
    std::string defaultKeyBinds[KEY_LENGTH] = KEY_BIND_NAMES;
    
    float scrollingBottomDist = 0.0;
    
    for (int i = 0; i<KEY_LENGTH; ++i){
        textGroupsStatic.push_back(defaultKeyBinds[i]);
        textGroupsStatic.push_back(INPUT_REGION);
        
        titleParams.push_back(baseKeyBindParam);
        
        titleParams.push_back(baseKeyBindInputParam);
        
        baseKeyBindParam.originY -= 0.25;
        baseKeyBindInputParam.originY -= 0.25;
        
        elements.push_back({{-0.3, 0.36f - 0.25f *i}, 3, 3});
        
        _numElementsButtons++;  // 0
        _numElementsDraggable = _numElementsButtons; // 0
        _numElementsTextFeilds = _numElementsDraggable; // 0
        
        scrollingBottomDist = 0.36f - 0.25f * i - 0.0175;
        
    }
    
    _stateXTextGroups = addTextGroups(textGroupsStatic, {}, {titleParams});
    
    for (int i = 0; i<KEY_LENGTH; ++i){
        setInputRegion(_stateXTextGroups[i*2+2], 0, keyCodeToStdString([_mtkView getKeyCodeAtIndex:i]));
    }

        
    _stateXElementSizes = [_device newBufferWithBytes:elementSizes
                                                      length:sizeof(elementSizes)
                                                     options:MTLResourceStorageModeShared];
    
    _stateXElementTextures = [_device newBufferWithBytes:elementTextures
                                                         length:sizeof(elementTextures)
                                                        options:MTLResourceStorageModeShared];
    
    _stateXElements = [_device newBufferWithBytes:elements.data()
                                                         length:elements.size()*sizeof(GUIVertex)
                                                        options:MTLResourceStorageModeShared];
    
    
    //NSError *error;
    
    MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:_device];
    
    NSDictionary *textureLoaderOptions =
    @{
        MTKTextureLoaderOptionTextureUsage       : @(MTLTextureUsageShaderRead),
        MTKTextureLoaderOptionTextureStorageMode : @(MTLStorageModePrivate)
    };
    
    _stateXTextureMap = [textureLoader newTextureWithName:@"KeyBinds-2"
                                              scaleFactor:1.0
                                                   bundle:nil
                                                  options:textureLoaderOptions
                                                    error:nullptr];

    [_mtkView setScrollingBottomDistTo:-scrollingBottomDist-1];
    
    recalculateCharacterAtlas(int(_mtkView.drawableSize.height));
    

}



void UIController::switchUIStateKeyBinds(){
    if ([_mtkView getKey:OPEN_KEYBINDS]){
        if (!(_selectedElement != 255 && _selectedElement >= _numElementsStatic && _selectedElement < _numElementsButtons)){
            if(_state!=0){
                _state = 0;
                popTextGroups(_stateXTextGroups);
                _selectedElement = 255;
                [_mtkView disableScrolling];
                _playerController->unlockCameraLockMouse();
                //[mtkView setInTextFeild:false];
            } else {
                _state = 2;
                loadKeyBinds();
                _playerController->lockCameraUnlockMouse();
                //[mtkView setInTextFeild:true];
            }
        }
        [_mtkView clearKey:OPEN_KEYBINDS];
    }
}



void UIController::stepUILogicKeyBinds(uint8_t currentBufferIndex){
    
    CGPoint mousePos = [_mtkView getMousePos];
    
    mousePos.x /= ((_aspect < 1.6f) ? 1/1.6 : (1/_aspect));
    mousePos.y /= ((_aspect > 1.6f) ? 1 : _aspect/1.6);
    
    mousePos.y -= *_scrollOffset;
    
    
    if (_selectedElement != 255 && _selectedElement >= _numElementsStatic && _selectedElement < _numElementsButtons){
        
        uint16_t lastKey = [_mtkView getLastKey];
        
        if(lastKey == 1000){
            return;
        }
        
        bool alreadyInList = false;
        for(int i = 0; i< KEY_LENGTH; ++i){
            if ([_mtkView getKeyCodeAtIndex:i] == uint8_t(lastKey)){
                alreadyInList = true;
                break;
            }
        }
        
        
        if (!alreadyInList){
            
            setInputRegion(_stateXTextGroups[(_selectedElement-_numElementsStatic)*2+2], 0, keyCodeToStdString(int(lastKey)));
            [_mtkView setKeyCodesAtIndex: int(_selectedElement-_numElementsStatic) to: uint8_t(lastKey)];
        }
        
        GUIVertex* tri = ((GUIVertex*)_stateXElements.contents)+_selectedElement;
        tri->uniformTextureIndex = 0;
        
        [_mtkView resetLastKey];
        
        _selectedElement = 255;
        
        if([_mtkView getKey:6]) {
            [_mtkView clearKey:6];
        }
        
        
        
        
    } else {
        checkIfButtonHover(mousePos, 0, 3, 1);
        
        
        if([_mtkView getKey:6]) { // mouse click
            
            checkIfInBounds(mousePos);
            
            GUIVertex* tri = ((GUIVertex*)_stateXElements.contents)+_selectedElement;
            clickButton(tri, mousePos, 0, 3, 1);
    //        std::cout << _selectedElement << "\n";
            
            [_mtkView resetLastKey];

        }
    }
    
    
    
    
}


// Draws all GUI related
void UIController::drawKeyBinds (id<MTLRenderCommandEncoder> renderEncoder, uint8_t currentBufferIndex)
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
    
    [renderEncoder setVertexBytes:_scrollOffset
                            length:sizeof(float)
                           atIndex:4];
    
    [renderEncoder setVertexBuffer:_stateXElementTextures
                            offset:0
                           atIndex:3];
    
    [renderEncoder setFragmentTexture:_stateXTextureMap atIndex:0];
    
    [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                              indexCount:6
                               indexType:MTLIndexTypeUInt16
                             indexBuffer:_squareIndexBuffer
                       indexBufferOffset:0
                           instanceCount:_numElementsTextFeilds];
    
    [renderEncoder popDebugGroup];
}
