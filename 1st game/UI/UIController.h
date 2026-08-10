



class UIController : public UIBackend {
public:
    
    UIController(id<MTLDevice> device, KeyboardMTKView* mtkView, PlayerController* playerController, id<MTLCommandQueue> commandQueue,
                 id<MTLRenderPipelineState> GUIPipelineState,id<MTLRenderPipelineState> textPipelineState,id<MTLRenderPipelineState> cursorPipelineState,id<MTLRenderPipelineState> highlightTextPipelineState, id<MTLComputePipelineState> fontCompute,
                 std::unordered_map<std::string, void*>* debugValues) : UIBackend(device, mtkView, playerController, commandQueue, GUIPipelineState, textPipelineState, cursorPipelineState, highlightTextPipelineState, fontCompute, debugValues) {
        loadDefault();
    }
    
    void loadDefault();
    void drawDefaultUI (id<MTLRenderCommandEncoder> renderEncoder, uint8_t currentBufferIndex);
    
    void loadNotes();
    void switchUIStateNotes();
    void stepUILogicNotes(uint8_t currentBufferIndex);
    void drawNotes (id<MTLRenderCommandEncoder> renderEncoder, uint8_t currentBufferIndex);
    
    void loadKeyBinds();
    void switchUIStateKeyBinds();
    void stepUILogicKeyBinds(uint8_t currentBufferIndex);
    void drawKeyBinds (id<MTLRenderCommandEncoder> renderEncoder, uint8_t currentBufferIndex);
    
    void loadDebug();
    void switchUIStateDebug();
    void stepUILogicDebug(uint8_t currentBufferIndex);
    void drawDebug (id<MTLRenderCommandEncoder> renderEncoder, uint8_t currentBufferIndex);

    
    
    
    
    
    
    
    void switchUIState() override {
        //switchUIStateFontDesigner(mtkView);
        switchUIStateNotes();
        switchUIStateKeyBinds();
        switchUIStateDebug();
        
    }
    
    void stepUILogic(uint8_t currentBufferIndex) override {
        

        
        switch(_state){
            case 0:
                //
                break;
            case 1:
                //stepUILogicFontDesigner(mtkView, currentBufferIndex);
                stepTyping();
                stepUILogicNotes(currentBufferIndex);
                break;
            case 2:

                stepUILogicKeyBinds(currentBufferIndex);
                
                break;
            case 3:
                stepUILogicDebug(0);
                switchUIStateDebug();
                break;
            default:
                std::cout << "\n\nError UI State: " << int(_state) << "\n\n";
                std::terminate();
        }
    }
    
    
    void drawUI (id<MTLRenderCommandEncoder> renderEncoder, uint8_t currentBufferIndex) override
    {
        
        [renderEncoder setVertexBytes:&_aspect
                                length:sizeof(_aspect)
                               atIndex:2];
        
        // Draw the UI that is always present (like health bar)
        drawDefaultUI(renderEncoder, currentBufferIndex);
        
        // Check for exclusive GUI states (like inventory)
        if (_state == 1) {
            drawNotes(renderEncoder, currentBufferIndex);
            
            [renderEncoder setRenderPipelineState:_textPipelineState];
            drawText(renderEncoder, currentBufferIndex);
            
            [renderEncoder setRenderPipelineState:_highlightTextPipelineState];
            drawCursor(renderEncoder);
            
        } else if (_state == 2) {
            //std::cout << "a\n";
            drawKeyBinds(renderEncoder, currentBufferIndex);
            
            [renderEncoder setRenderPipelineState:_textPipelineState];
            drawText(renderEncoder, currentBufferIndex);
            
        } else if (_state == 3) {
            [renderEncoder setRenderPipelineState:_textPipelineState];
            drawText (renderEncoder, currentBufferIndex);
            //drawCursor(renderEncoder, _highlightTextPipelineState, _cursorPipelineState);
        }
        
    }
};
