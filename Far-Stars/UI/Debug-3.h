/*
 Debug (3):
 
    A screen to monitor stats.

  - Text:
     - States version, fps, time to render frame, position, and screen aspect ratio

*/


void UIController::loadDebug(){
    
    
    std::vector<std::string> textGroupsStatic = {"Version 0.1.0 \n\nFPS: " INPUT_REGION "\nTime to render frame: " INPUT_REGION "\n\nPos (XYZ): " INPUT_REGION ", " INPUT_REGION ", " INPUT_REGION "\n\nAspect Ratio: " INPUT_REGION};
    
    textPerameters params = {MAX_LENGTH_LINES_FLAG, simd_make_float3(1,1,1), 3, -1.5, 0.75, 0.1, 0 ,0, 20};
    
    _stateXTextGroups = addTextGroups(textGroupsStatic, {}, {params});
    
    recalculateCharacterAtlas(int(_mtkView.drawableSize.height));
}



void UIController::switchUIStateDebug(){
    
    if ([_mtkView getKey:OPEN_DEBUG_PANEL]){
        if (!(_selectedElement != 255 && _selectedElement >= _numElementsStatic && _selectedElement < _numElementsButtons)){
            if(_state!=0){
                _state = 0;
                popTextGroups(_stateXTextGroups);
            } else {
                _state = 3;
                loadDebug();
                
            }
        }
        [_mtkView clearKey:OPEN_DEBUG_PANEL];
    }
}



void UIController::stepUILogicDebug(uint8_t currentBufferIndex){
    
    if((*_debugValues).contains("fps")){
        setInputRegion(0, 0, std::to_string(int(1000000 / (*(float*)((*_debugValues)["fps"])))));
    }
    
    if((*_debugValues).contains("time")){
        setInputRegion(0, 1, std::to_string(((std::chrono::microseconds*)((*_debugValues)["time"]))->count()));
    }
    
    // std::unordered_map<std::string, void*> *_debugValues;
    setInputRegion(0, 2, std::to_string((*(simd_float3*)((*_debugValues)["pos"])).x));
    setInputRegion(0, 3, std::to_string((*(simd_float3*)((*_debugValues)["pos"])).y));
    setInputRegion(0, 4, std::to_string((*(simd_float3*)((*_debugValues)["pos"])).z));
    
    setInputRegion(_stateXTextGroups[0], 5, std::to_string(_aspect));
    
}


// Draws all GUI related
void UIController::drawDebug (id<MTLRenderCommandEncoder> renderEncoder, uint8_t currentBufferIndex)
{

}

