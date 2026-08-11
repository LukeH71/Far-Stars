/*
 Default (0):
 
    No UI
 
  
*/




void UIController::loadDefault(){
    
    _numElementsStatic = 0;
    _numElementsButtons = _numElementsStatic;
    _numElementsDraggable = _numElementsButtons;
    _numElementsTextFeilds = _numElementsDraggable;
    
    
    static const GUIVertex UIMesh[] = {
        
        { { -1, -1 }, 0 }
        
    };
    
    static const GUIVertexUniforms UIUniforms[] = {
        { {2, 2} }
    };
    
    static const GUITextureUniforms UITexUniforms[] = {
        { {0, 0}, {1, 1} }
    };
    
    //_values.push_back(30);
    

        
    _state0ElementSizes = [_device newBufferWithBytes:UIUniforms
                                                      length:sizeof(UIUniforms)
                                                     options:MTLResourceStorageModeShared];
    
    _state0ElementTextures = [_device newBufferWithBytes:UITexUniforms
                                                         length:sizeof(UITexUniforms)
                                                        options:MTLResourceStorageModeShared];
    
    _state0Elements = [_device newBufferWithBytes:UIMesh
                                                         length:sizeof(UIMesh)
                                                        options:MTLResourceStorageModeShared];
    
    
}


// Draws all the transparent meshes from the transparent actors array.
void UIController::drawDefaultUI (id<MTLRenderCommandEncoder> renderEncoder, uint8_t currentBufferIndex)
{

}
