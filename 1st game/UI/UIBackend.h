

// _ / 8 = num chars round by (256 / 8 = 32 chars)
#define TEXT_BUFFER_ALIGN_TO 256

#define DISABLE_EDIT_FLAG     0b00000001
#define MAX_LENGTH_LINES_FLAG 0b00000010
#define DISABLE_ENTER_FLAG    0b00000100
#define INTEGER_ONLY_FLAG     0b00001000
#define FLOAT_ONLY_FLAG       0b00010000

#define INPUT_REGION "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"

#define TextLettersPerLine 14
#define TextNumbersLines 7

#define NewlineDist 1.35f




// General class for handling UI
class UIBackend {
    
#pragma mark -
#pragma mark Variables and Variable Types
    
    
public:
    
    typedef struct {
        
        uint8_t flags;
        
        simd_float3 color;
        
        float lineWrappingLength;
        float originX, originY;
        float fontSize;
        
        uint16_t startsAt;
        uint16_t textEndsAt;
        
        uint8_t maxLength;

    } textPerameters;

    enum ModificationTypes {
        ADD,
        DELETE,
        ARROW,
        SHIFT_ARROW,
        SKIP
    };
    
protected:

    
    typedef struct {
        simd_float3 color;
        float fontSize;
        uint16_t startsAt;
    } truncatedTextPerameters;
    
    typedef struct {
        int heightPerLetter;
        int widthOfPadding;
    } characterAtlasParams;
    
    typedef struct
    {
        vector_float2 position; // -x, -y
        uint8_t uniformVertexIndex;
        uint8_t uniformTextureIndex;
    } GUIVertex; // 10
    
    typedef struct
    {
        vector_float2 sizes;
    } GUIVertexUniforms; // 24

    typedef struct
    {
        vector_float2 texCoords;
        vector_float2 texSizes;
    } GUITextureUniforms; // 24
    
    typedef struct {
        float coords[6];
    } bezierCurve;
    
    typedef struct {
        float sizeX, sizeY;
        float minusX, minusY;
        uint8_t numSnapsX, numSnapsY;
    } snapping;
    
    enum class TextOrderer : uint8_t { ADDITION, ORIGINAL};
    
    typedef struct {
        TextOrderer source;
        int startOfGroup;
        int endOfGroup;
    } textOrderer;
    
    
    const std::unordered_map<uint16_t, float> kernsSingle = PGB_KERNING_SINGLE;
    
    // Variables for all UI states
    uint8_t _state;
    id<MTLDevice> _device;
    float _aspect;
    float *_scrollOffset;
    
    // The UI that is constantly on the screen (i.e. MC hotbar)
    id <MTLBuffer> _state0Elements; // Has the location of an element, what is the size, and what texture it is using (below 2 vars)
    id <MTLBuffer> _state0ElementSizes;
    id <MTLBuffer> _state0ElementTextures; // Has the texture coordinates of an element, referring to the texture below
    id <MTLTexture> _state0TextureMap;
    std::vector<int> _state0TextGroups;
    
    // Re-initiate this buffer whenever it is needed to be larger
    // Otherwise, keep it the same and reuse it for each different
    // UI states
    id <MTLBuffer> _stateXElements; // Has the location of an element, what is the size, and what texture it is using (below 2 vars)
    id <MTLBuffer> _stateXElementSizes;
    id <MTLBuffer> _stateXElementTextures; // Has the texture coordinates of an element, referring to the texture below
    id <MTLTexture> _stateXTextureMap;
    std::vector<int> _stateXTextGroups;
    
    // The values below are used to describe how to add logic to a UI screen
    int _selectedElement;
    int _numElementsStatic;
    int _numElementsButtons;
    int _numElementsDraggable;
    int _numElementsTextFeilds;
    
    id <MTLBuffer> _textBuffer;
    id <MTLBuffer> _textPositioningBuffer;
    
    id <MTLBuffer> _characterWidthBuffer;
    id <MTLBuffer> _textParamsBuffer;
    
    id <MTLBuffer> _letterTexCoords;
    
    simd_float2 _letterDims;
    id<MTLTexture> _characterAtlas;
    
    std::vector<textPerameters> _textParams;
    std::vector<std::vector<int>> _fields;
    
    uint16_t _indexOfTextParams;
    uint16_t _textBufferLength;
    uint16_t _cursorIndex;
    uint16_t _highlightStartIndex; // 0 = not highlight, shift index by 1
    uint16_t _staticMargin;
    
    
    // Index wrapping buffers for more efficient rendering in inctancing
    id <MTLBuffer> _squareIndexBuffer;
    
    float _cursorBlink;
    bool _mouseClickCkeck;
    

    KeyboardMTKView *_mtkView;
    PlayerController *_playerController;
    
    id<MTLRenderPipelineState> _GUIPipelineState;
    
    id<MTLRenderPipelineState> _textPipelineState;
    id<MTLRenderPipelineState> _cursorPipelineState;
    id<MTLRenderPipelineState> _highlightTextPipelineState;

    id<MTLComputePipelineState> _fontCompute;
    
    id<MTLCommandQueue> _commandQueue;
    
    
    std::unordered_map<std::string, void*> *_debugValues;
    
    
    
    
    
public:
    

#pragma mark -
#pragma mark Initiator
    
    
    
    //
    UIBackend(id<MTLDevice> device, KeyboardMTKView* mtkView, PlayerController* playerController, id<MTLCommandQueue> commandQueue,
              id<MTLRenderPipelineState> GUIPipelineState,id<MTLRenderPipelineState> textPipelineState,id<MTLRenderPipelineState> cursorPipelineState,id<MTLRenderPipelineState> highlightTextPipelineState, id<MTLComputePipelineState> fontCompute,
              std::unordered_map<std::string, void*>* debugValues) {
        
        _state = 0;
        
        _device = device;
        
        _mtkView = mtkView;
        _playerController = playerController;
        _playerController->unlockCameraLockMouse();
        
        _commandQueue = commandQueue;
        
        _GUIPipelineState = GUIPipelineState;
        
        _textPipelineState = textPipelineState;
        _cursorPipelineState = cursorPipelineState;
        _highlightTextPipelineState = highlightTextPipelineState;
        
        _fontCompute = fontCompute;
        
        _debugValues = debugValues;
        
        
        
        
//        // Create 256-byte aligned text groups
//        
//        std::vector<uint8_t> text = {}; // Container for all of the text to be put into a buffer
//
//        for(int i = 0; i< textStrings.size(); ++i){ // Loop over each new text group
//            
//            // Store a text group's text and put it in 'textGroup' w/ a leading and trailing 0
//            std::vector<uint8_t> textGroup = {0};
//            stdStringToText(&textGroup, textStrings[i], i);
//            textGroup.push_back(0);
//            
//            uint alignedSize = alignTo256(textGroup.size());
//            uint textEndsAt = uint(textGroup.size());
//            
//            // Align 'textGroup's size to 256-bytes for buffer optimization and less reinitiation
//            for (int j = 0; j< alignedSize - textEndsAt; ++j){
//                textGroup.push_back(0);
//            }
//            
//            _textParams.push_back(textParameters[i]);
//            _textParams.back().startsAt = text.size();
//            _textParams.back().textEndsAt = text.size() + textEndsAt;
//            
//            text.insert(text.end(), textGroup.begin(), textGroup.end());
//        }
        
        //_cursorIndex = _textParams.back().textEndsAt-1;
        _cursorIndex = 0;
        _highlightStartIndex = 0;
        
        _textBufferLength = 0;
        _indexOfTextParams = 0;
        
        _scrollOffset = [_mtkView getScrollingWheelDeltaY];
        //_textBufferLength = text.size();
    
        //_indexOfTextParams = _textParams.size()-1;
        
        
        // Initiate buffers to be used on each text draw call

        float letterWidths[] = PGB_WIDTHS; // optimise
        
        _characterWidthBuffer = [_device newBufferWithBytes:&letterWidths
                                                     length:sizeof(letterWidths)
                                                    options:MTLResourceStorageModeShared];
        
//        _textBuffer = [_device newBufferWithBytes:text.data()
//                                           length:text.size()*sizeof(uint8_t)
//                                             options:MTLResourceStorageModeShared];
//        
//        _textPositioningBuffer = [_device newBufferWithLength:text.size()*sizeof(vector_float2)
//                                                options:MTLResourceStorageModeShared];
        
//        
//        std::vector<truncatedTextPerameters> truncatedTextParams;
//        for(int m = 0; m<_textParams.size(); ++m){
//            truncatedTextParams.push_back({_textParams[m].color, _textParams[m].fontSize, _textParams[m].startsAt});
//        }
//        
//        _textParamsBuffer = [_device newBufferWithBytes:truncatedTextParams.data()
//                                                 length:truncatedTextParams.size() * sizeof(truncatedTextPerameters)
//                                                options:MTLResourceStorageModeShared];
        
//        
//        // Properly apply spacing to each text group
//        for(int i = 0; i<_textParams.size(); ++i){
//            stepSpacing(_textParams[i].startsAt+1, i);
//        }
//        
        
        // Create an index buffer of a square used to render each letter
        static const uint16_t indices[] = { 0, 1, 2, 1, 3, 2 };
        _squareIndexBuffer = [_device newBufferWithBytes:indices
                                                  length:sizeof(indices)
                                                 options:MTLResourceStorageModeShared];
        
        _staticMargin = 0;
        //addTextGroups({"Apple!!!"}, {}, {{MAX_LENGTH_LINES_FLAG, simd_make_float3(0,0,0), 3, -1.5, 0.75, 0.2, 0 , 5, 20}});
        
        //popTextGroups({1});
    }
    
    
    std::vector<int> addTextGroups (std::vector<std::string> textStringsStatic, std::vector<std::string> textStringsEditable, std::vector<textPerameters> textParameters){
        
        std::vector<std::vector<uint8_t>> textStatic = {};
        std::vector<std::vector<uint8_t>> textEditable = {};
        
        int summedSize = 0;
        for(int i = 0; i<textStringsStatic.size(); ++i){
            summedSize += alignTo256(textStringsStatic[i].length());
            textStatic.push_back({0});
            
            stdStringToText(&(textStatic[i]), textStringsStatic[i], _staticMargin + i);
            
            textStatic.back().push_back(0);
        }
        for(int i = 0; i<textStringsEditable.size(); ++i){
            summedSize += alignTo256(textStringsEditable[i].length());
            
            textEditable.push_back({0});
            stdStringToText(&(textEditable[i]), textStringsEditable[i], int(_textParams.size()+textStringsStatic.size()+i));
            
            textEditable.back().push_back(0);
        }
        
        int totalSize = summedSize + _textBufferLength;
        
        id<MTLBuffer> oldTextBuffer = _textBuffer;
        id<MTLBuffer> oldSpacingBuffer = _textPositioningBuffer;
        
        _textBuffer = [_device newBufferWithLength:totalSize*sizeof(uint8_t) options:MTLStorageModeShared];
        _textPositioningBuffer = [_device newBufferWithLength:totalSize*sizeof(simd_float2) options:MTLStorageModeShared];
        
        int typedLength = 0;
        int globalIndex = 0;
        
        std::vector<int> indicies = {};
        
        std::vector<textPerameters> oldTextParams = _textParams;
        _textParams.clear();
        
        for(int i = 0; i<_staticMargin; ++i){
            int length = alignTo256(oldTextParams[i].textEndsAt - oldTextParams[i].startsAt);
            memcpy((uint8_t*)_textBuffer.contents + typedLength, (uint8_t*)oldTextBuffer.contents + oldTextParams[i].startsAt, length);
            memcpy((simd_float2*)_textPositioningBuffer.contents + typedLength, (uint8_t*)oldSpacingBuffer.contents + oldTextParams[i].startsAt, length * sizeof(simd_float2));
            typedLength += length;
            globalIndex+=1;
            
            
            _textParams.push_back(oldTextParams[i]);
        }
        
        for(int i = 0; i<textStatic.size(); ++i){
            int length = int(textStatic[i].size());
            memcpy((uint8_t*)_textBuffer.contents + typedLength, textStatic[i].data(), length);
            //memcpy((simd_float2*)_textPositioningBuffer.contents + typedLength, textStringsStatic[i].data(), textStringsStatic[i].length() * sizeof(simd_float2));
            
            
            indicies.push_back(globalIndex);
            
            globalIndex+=1;
            
            _textParams.push_back({uint8_t(textParameters[i].flags | DISABLE_EDIT_FLAG), textParameters[i].color, textParameters[i].lineWrappingLength, textParameters[i].originX, textParameters[i].originY, textParameters[i].fontSize,
                uint16_t(typedLength),
                uint16_t(typedLength+length),
                textParameters[i].maxLength});
            
            typedLength += alignTo256(length);
        }
        
        int newStaticMargin = int(_textParams.size());
        
        for(int i = _staticMargin; i<oldTextParams.size(); ++i){
            int length = alignTo256(oldTextParams[i].textEndsAt - oldTextParams[i].startsAt);
            memcpy((uint8_t*)_textBuffer.contents + typedLength, (uint8_t*)oldTextBuffer.contents + oldTextParams[i].startsAt, length);
            memcpy((simd_float2*)_textPositioningBuffer.contents + typedLength, (uint8_t*)oldSpacingBuffer.contents + oldTextParams[i].startsAt, length * sizeof(simd_float2));
            typedLength += length;
            globalIndex += 1;
            
            _textParams.push_back(oldTextParams[i]);
        }
        
        for(int i = 0; i<textEditable.size(); ++i){
            int paramI = int(i+textStatic.size());

            int length = int(textEditable[i].size());
            memcpy((uint8_t*)_textBuffer.contents + typedLength, textEditable[i].data(), length);
            
            indicies.push_back(globalIndex);
            
            globalIndex += 1;
            
            _textParams.push_back({uint8_t(textParameters[paramI].flags), textParameters[paramI].color, textParameters[paramI].lineWrappingLength, textParameters[paramI].originX, textParameters[paramI].originY, textParameters[paramI].fontSize,
                uint16_t(typedLength),
                uint16_t(typedLength+length),
                textParameters[paramI].maxLength});
            
            typedLength += alignTo256(length);
        }
        
        
        _cursorIndex = 0;
        _highlightStartIndex = 0;
        _indexOfTextParams = 0;
        
        _textBufferLength += summedSize;
        
        _staticMargin = newStaticMargin;
        
        
        std::vector<truncatedTextPerameters> truncatedTextParams;
        for(int m = 0; m<_textParams.size(); ++m){
            truncatedTextParams.push_back({_textParams[m].color, _textParams[m].fontSize, _textParams[m].startsAt});
        }
        
        _textParamsBuffer = [_device newBufferWithBytes:truncatedTextParams.data()
                                                 length:truncatedTextParams.size() * sizeof(truncatedTextPerameters)
                                                options:MTLResourceStorageModeShared];
        
        // Properly apply spacing to each text group
        for(int i = 0; i<indicies.size(); ++i){
            stepSpacing(_textParams[indicies[i]].startsAt+1, indicies[i]);
        }
        
        
        return indicies;
    }
    
    void popTextGroups (std::vector<int> indicies){
        
        int summedSize = 0;
        int numBelowStaticMargin = 0;
        for(int i = 0; i<indicies.size(); ++i){
            summedSize += alignTo256(_textParams[indicies[i]].textEndsAt - _textParams[indicies[i]].startsAt);
            
            if (i < _staticMargin){
                numBelowStaticMargin += 1;
            }
        }
        
        int totalSize = _textBufferLength - summedSize;
        
        id<MTLBuffer> oldTextBuffer = _textBuffer;
        id<MTLBuffer> oldSpacingBuffer = _textPositioningBuffer;
        
        if (totalSize > 0) {
            _textBuffer = [_device newBufferWithLength:totalSize*sizeof(uint8_t) options:MTLStorageModeShared];
            _textPositioningBuffer = [_device newBufferWithLength:totalSize*sizeof(simd_float2) options:MTLStorageModeShared];
        }
        
        _staticMargin -= numBelowStaticMargin;
        
        
        std::vector<textPerameters> oldTextParams = _textParams;
        _textParams.clear();
        
        std::vector<std::vector<int>> oldFields = _fields;
        _fields.clear();
        
        int distanceErased = 0;
        int typedLength = 0;
        
        int nextIndex = 0;
        
        for(int i = 0; i<oldTextParams.size(); ++i){
            if(i == indicies[nextIndex]){
                
                
                distanceErased += oldTextParams[i].textEndsAt - oldTextParams[i].startsAt;
                
                
                nextIndex += 1;
                if (nextIndex > indicies.size()){
                    nextIndex = 0;
                }
            } else {
                int length = alignTo256(oldTextParams[i].textEndsAt - oldTextParams[i].startsAt);
                
                memcpy((uint8_t*)_textBuffer.contents + typedLength, (uint8_t*)oldTextBuffer.contents + typedLength + distanceErased, length);
                memcpy((simd_float2*)_textPositioningBuffer.contents + typedLength, (simd_float2*)oldSpacingBuffer.contents + typedLength + distanceErased, length * sizeof(simd_float2));
                
                _textParams.push_back({uint8_t(oldTextParams[i].flags), oldTextParams[i].color, oldTextParams[i].lineWrappingLength, oldTextParams[i].originX, oldTextParams[i].originY, oldTextParams[i].fontSize,
                    uint16_t(typedLength),
                    uint16_t(typedLength+(oldTextParams[i].textEndsAt - oldTextParams[i].startsAt)),
                    oldTextParams[i].maxLength});
                
                typedLength += length;
                
                _fields.push_back(oldFields[i]);
            }
        }
        
        
        
        
        _textBufferLength -= summedSize;
        
        _cursorIndex = 0;
        _highlightStartIndex = 0;
        _indexOfTextParams = 0;
        
        
        
        
        std::vector<truncatedTextPerameters> truncatedTextParams;
        for(int m = 0; m<_textParams.size(); ++m){
            truncatedTextParams.push_back({_textParams[m].color, _textParams[m].fontSize, _textParams[m].startsAt});
        }
        
        if (truncatedTextParams.size() > 0){
            _textParamsBuffer = [_device newBufferWithBytes:truncatedTextParams.data()
                                                     length:truncatedTextParams.size() * sizeof(truncatedTextPerameters)
                                                    options:MTLResourceStorageModeShared];
        }
        

    }
    
    
    
    // Set a specified region to a value (int / float / anything <= 20 chars and cast as a string)
    void setInputRegion(int group, int index, std::string newText, bool skipSpacing = false){
        
        if(_fields.size() <= group){
            return;
        } else if (_fields[group].size() <= index){
            return;
        }
        
        int start = _fields[group][index] + _textParams[group].startsAt;
        
        std::vector<uint8_t> formattedText;
        stdStringToText(&formattedText, newText);
        
        while(formattedText.size()<20){
            formattedText.push_back(0);
        }
        
        // Do not set if the value is already that
        if(memcmp((uint8_t*)_textBuffer.contents+start, formattedText.data(), 20) == 0) {
            return;
        }
        
        memcpy((uint8_t*)_textBuffer.contents+start, formattedText.data(), 20);
        
        
        if (!skipSpacing){
            stepSpacing(start-1, group);
        }
    }
    
    
    // Setter for aspect ratio
    void setAspect (float newAspect) {
        _aspect = newAspect;
    }
        
    int alignTo256(uint16_t size){
        return ((size/256)+1)*256;
    }
    
    
    
    
#pragma mark -
#pragma mark Overridden UI State Logic
    
protected:
    
    virtual void switchUIState(){
        std::terminate();
    }
    
    virtual void stepUILogic(uint8_t currentBufferIndex){
        std::terminate();
    }
    
public:
    
    // A general function that can be called to handle all GUI related logic
    void GUILogic(uint8_t currentBufferIndex){
        
        switchUIState();
        stepUILogic(currentBufferIndex);
        
    }
    
protected:
    
    
#pragma mark -
#pragma mark UI Element Logic
    
    // A typical use of UI logic is:
    //   checkIfInBounds(currentBufferIndex, mousePos);
    //      - sets '_selectedElement' to the element the mouse is selecting
    //   GUIVertex* tri = ((GUIVertex*)_stateXElements.contents)+_selectedElement;
    //      - gets a pointer to the geometry of the selected element
    //   clickButton(tri, mousePos);
    //      - runs button-pressing logic
    //   ...
    //   switch(deselectButton(currentBufferIndex, mousePos)){ ... }
    //      - allows actions to be evaluated after a button is released
    
    // A '_selectedElement' < 0 means that nothing is selected
    
    
    bool insideBounds(GUIVertex* tri, CGPoint mousePos){
        
        vector_float2 pos = tri->position;
        vector_float2 size = (((GUIVertexUniforms*)_stateXElementSizes.contents)+tri->uniformVertexIndex)->sizes;
        
        return (mousePos.x>pos.x&&mousePos.x<pos.x+size.x && mousePos.y>pos.y&&mousePos.y<pos.y+size.y);
    }
    
    
    void checkIfInBounds(CGPoint mousePos) {
            
        for (int i = _numElementsStatic; i< _numElementsTextFeilds; ++i) {
            
            GUIVertex* tri = ((GUIVertex*)_stateXElements.contents)+i;
    
            if (insideBounds(tri, mousePos)){
                _selectedElement = i;
                return;
            }
        }
        
        _selectedElement = -1;
        
    }
    
    
#pragma mark draggable logic
    
    // A function called when the mouse is clicked
    simd_float2 moveDot(GUIVertex* tri, vector_float2 size, CGPoint mousePos){
        
        if (_selectedElement >= _numElementsButtons && _selectedElement < _numElementsDraggable) {
            
            simd_float2 start = tri->position;
            tri->position = {static_cast<float>(mousePos.x)-size.x/2, static_cast<float>(mousePos.y)-size.y/2};
            
            return start - tri->position;
        }
        
        return simd_make_float2(0, 0);
    }
    
    // A function called when the mouse is unclicked
    simd_float2 deselectDot(GUIVertex* tri, std::vector<snapping> snaps){
        
        if (_selectedElement >= _numElementsButtons && _selectedElement < _numElementsDraggable) {
            
            simd_float2 start = tri->position;
            
            for(uint8_t i = 0; i<snaps.size(); ++i){
                
                tri->position.x -= snaps[i].minusX;
                tri->position.x = (snaps[i].sizeX / snaps[i].numSnapsX) * int( tri->position.x / (snaps[i].sizeX / snaps[i].numSnapsX ) + 0.5 );
                tri->position.x += snaps[i].minusX;
                
                tri->position.y -= snaps[i].minusY;
                tri->position.y = (snaps[i].sizeY / snaps[i].numSnapsY) * int(tri->position.y / (snaps[i].sizeY / snaps[i].numSnapsY) + 0.5);
                tri->position.y += snaps[i].minusY;
            }
            
            return start - tri->position;
        }
        
        return simd_make_float2(0, 0);
    }
    
#pragma mark button logic
    
    // A function called when the mouse is clicked
    void clickButton(GUIVertex* tri, CGPoint mousePos, int texUniformDefault, int texUniformHover, int texUniformClick){
        if (_selectedElement >= _numElementsStatic && _selectedElement < _numElementsButtons) {
            if(insideBounds(tri, mousePos)) {
                tri->uniformTextureIndex = texUniformClick;
            } else {
                tri->uniformTextureIndex = texUniformDefault;
            }
        }
    }
    
    void checkIfButtonHover(CGPoint mousePos, int texUniformDefault, int texUniformHover, int texUniformClick){
        for (int i = _numElementsStatic; i<_numElementsButtons; ++i) {
            
            GUIVertex* tri = ((GUIVertex*)_stateXElements.contents)+i;
            
            if (insideBounds(tri, mousePos)){
                tri->uniformTextureIndex = texUniformHover;
            } else {
                tri->uniformTextureIndex = texUniformDefault;
            }
        }
    }
    
    // A function called when the mouse is unclicked
    int deselectButton(CGPoint mousePos){
        
        mousePos.x /= ((_aspect < 1.6f) ? 1/1.6 : (1/_aspect));
        mousePos.y /= ((_aspect > 1.6f) ? 1 : _aspect/1.6);
        
        if (_selectedElement >= _numElementsStatic && _selectedElement < _numElementsButtons) {

            GUIVertex* tri = ((GUIVertex*)_stateXElements.contents)+_selectedElement;
            
            if (insideBounds(tri, mousePos)){
                
                GUIVertex* presentDot = (GUIVertex*)_stateXElements.contents;
                (presentDot+_selectedElement)->uniformTextureIndex = 4;
                
                
                return _selectedElement - _numElementsStatic;
            }
            
            GUIVertex* presentDot = (GUIVertex*)_stateXElements.contents;
            (presentDot+_selectedElement)->uniformTextureIndex = 2;
            
            
        }
        
        
        
        return -1;
    }
    
#pragma mark text feild logic
    
    // A function called when the mouse is clicked
    int clickTextFeild(CGPoint mousePos){
        if (_selectedElement >= _numElementsDraggable && _selectedElement < _numElementsTextFeilds) {
            
            uint16_t selectedTextParam = _numElementsTextFeilds - _selectedElement;
            
            float lineWrappingLength =  _textParams[selectedTextParam-1].lineWrappingLength;
            
            
            int end = _textParams[selectedTextParam-1].textEndsAt;
            vector_float2* spacing = (vector_float2*)_textPositioningBuffer.contents;
            
            mousePos.y+=0.5*_textParams[selectedTextParam-1].fontSize;
            
            _cursorIndex = end;
            float closestDist = -1;
            for (int i = _textParams[selectedTextParam-1].startsAt+1; i<end+1; ++i){
                
                float spacingX = mousePos.x;
                float spacingY = int((spacing[i].y-(_textParams[selectedTextParam-1].originY)) / (_textParams[selectedTextParam-1].fontSize*NewlineDist)) * (_textParams[selectedTextParam-1].fontSize*NewlineDist) + (_textParams[selectedTextParam-1].originY);
                //float spacingY = int(spacing[i].y / _textParams[selected-1].fontSize*NewlineDist) * _textParams[selected-1].fontSize*NewlineDist;
                spacingY += _textParams[selectedTextParam-1].fontSize;
                
                float dx = (spacing[i].x-spacingX);
                float dy = (spacingY-mousePos.y);
                
                // dx^2 + (dy^2)(lwl^4)
                float dist = dx*dx +dy*dy *lineWrappingLength*lineWrappingLength*lineWrappingLength*lineWrappingLength;
                
                //std::cout << dist << " ";
                if(dist<closestDist || closestDist < 0){
                    _cursorIndex = i;
                    closestDist = dist;
                }
            }
            
            int originalTextParam = _indexOfTextParams;
            
            _indexOfTextParams = 0;
            while(_indexOfTextParams+1<_textParams.size() && _textParams[_indexOfTextParams+1].startsAt<_cursorIndex) {
                ++_indexOfTextParams;
            }
            
            // Stop highlighting from one text param to another
            if(originalTextParam!=_indexOfTextParams){
                _highlightStartIndex = _cursorIndex+1;
            }
            
            if (!_mouseClickCkeck){
                _mouseClickCkeck = true;
                _highlightStartIndex = MIN(MAX(_cursorIndex + 1, _textParams[_indexOfTextParams].startsAt), _textParams[_indexOfTextParams].textEndsAt);
            }
            
            _cursorIndex = MIN(MAX(_cursorIndex, _textParams[_indexOfTextParams].startsAt), _textParams[_indexOfTextParams].textEndsAt-1);
            
            //std::cout << "\n" << end-_cursorIndex << "\n\n";
            _cursorBlink = 1;
            
            
            return selectedTextParam;
        }
        return -1;
    }
    
    
    
#pragma mark -
#pragma mark TEXT
    
    
    
#pragma mark Text Init / Getters & Setters
    
public:
    
    // Rasterize characters into a catalogue, and when rendering text, reference the catalogue. Size = height per letter in pixels
    void loadTextSize(int size){
        
        size = MIN(1000, size);
        size = MAX(25, size);
        
        // Have 5 pixel padding border inbetween letters. This prevents seeing the border of one letter in the border of another
        int padSize = 5;
        
        int lettersPerLine = TextLettersPerLine;
        int numLines = TextNumbersLines;
        
        // The widths of each letters
        std::vector<float> letterWidths = PGB_WIDTHS;
        
        const int numCharsNeedToRender = 99;
        
        // Get the summation of the width and padding of every letter needed to rasterize
        int totalWidthLettersSum = 0;
        for(int i = 1; i<numCharsNeedToRender; ++i){
            totalWidthLettersSum += size * abs(letterWidths[i]) + padSize;
        }
        
        
        // The witdth in pixels of each letter, given as a buffer. One can find what letter they are in by finding spacings[pixel.x]
        id<MTLBuffer> spacingsBuffer = [_device newBufferWithLength:totalWidthLettersSum + padSize * numCharsNeedToRender
                                              options:MTLResourceStorageModeShared];
        
        memset((uint8_t*)spacingsBuffer.contents, 0, totalWidthLettersSum + padSize * numCharsNeedToRender); // Zero init it
        
        
        // BUT not all pixels are on the same x level. To find the index for spacings, one can use "jumps". Jumps stores the distance each line holds.
        // by adding the jumps to the x axis on the current line, the index may be found (this creates a "global" x coordinate).
        
        
        // Units all in pixels
        int distance = 0; // The current global distance travled
        int lineDistance = 0; // The current distance on one line
        int maxDistance = 0; // The maximum distance traveleed by any one particular line (used to create texture / find texture dims)
        
        std::vector<float> distances; // Each (global) distance per letter
        std::vector<float> jumps; // The jumps value, stores per line distance of 10 letters per line
    
        // Letter zero adds 0 distance
        distances.push_back(0);
        jumps.push_back(0);
        
        // Loop over each letter (skip letter zero by starting at i=1)
        for(uint8_t i = 1; i<numCharsNeedToRender; ++i){
            int stride = size * abs(letterWidths[i]) + padSize; // distance travled per letter
            
            memset((uint8_t*)spacingsBuffer.contents+distance, i, stride);
            
            // Every lettersPerLine, jump to a new line. Add to jumps, and reset line distance.
            if(i%(lettersPerLine)==0){
                if(lineDistance>maxDistance) maxDistance = lineDistance;
                jumps.push_back(jumps[jumps.size()-1]+lineDistance);
                lineDistance = 0;
            }
            
            distance += stride;
            lineDistance += stride;
            distances.push_back(distances[i-1]+stride);
        }
        
        
        
        
        distances.push_back(distances[numCharsNeedToRender]);

        
        // Calculate coordinates for each letter
        std::vector<simd_float2> letterCoordinates;
        letterCoordinates.push_back(simd_make_float2(0,0));
        
        for(int i = 1; i<distances.size(); ++i) {
            simd_float2 newValue;
            newValue.x = (distances[i-1]-jumps[(i)/lettersPerLine])/maxDistance;
            newValue.y = float((int(i/lettersPerLine)))/numLines;
            
            letterCoordinates.push_back(newValue);
        }
        
        
        // Establish buffers
        
        int dataDividing[] = PGB_DIVIDING;
        bezierCurve curve[] = PGB_DATA;
        
        characterAtlasParams atlasParams = {size, padSize};
        
        
        // Local buffers just for this gpu call
        id<MTLBuffer> distancesBuffer = [_device newBufferWithBytes:distances.data()
                                           length:distances.size() * sizeof(float)
                                              options:MTLResourceStorageModeShared];
        
        id<MTLBuffer> jumpsBuffer = [_device newBufferWithBytes:jumps.data()
                                           length:jumps.size() * sizeof(float)
                                              options:MTLResourceStorageModeShared];
        
        id<MTLBuffer> bezierCurvesBuffer = [_device newBufferWithBytes:&curve
                                                      length:sizeof(curve)
                                                     options:MTLResourceStorageModeShared];
        
        id<MTLBuffer> dividingBuffer =[_device newBufferWithBytes:&dataDividing
                                                    length:sizeof(dataDividing)
                                                   options:MTLResourceStorageModeShared];
        
        // Global buffers for later use
        _letterTexCoords = [_device newBufferWithBytes:letterCoordinates.data()
                                                   length:letterCoordinates.size() * sizeof(simd_float2)
                                                      options:MTLResourceStorageModeShared];
        
        _letterDims = simd_make_float2(float(size+padSize)/maxDistance,float(size)/((size+padSize)*numLines));
        
        
        
        
        // Create the texture as (maxDistance * calculated height)
        MTLTextureDescriptor *desc =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                                               width:maxDistance
                                                              height:(size+padSize)*numLines
                                                            mipmapped:NO];

        
        desc.usage = MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead;
        desc.storageMode = MTLStorageModePrivate;

        _characterAtlas = [_device newTextureWithDescriptor:desc];
        
        
        
        
        // Call a compute shader to prerasterize each letter for later reference
        
        id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
        
        id<MTLComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoderWithDispatchType:MTLDispatchTypeConcurrent];
        
        [computeEncoder setComputePipelineState:_fontCompute];
        
        
        [computeEncoder setBuffer:bezierCurvesBuffer offset:0 atIndex:0];
        [computeEncoder setBuffer:dividingBuffer offset:0 atIndex:1];
        [computeEncoder setBuffer:spacingsBuffer offset:0 atIndex:2];
        [computeEncoder setBuffer:distancesBuffer offset:0 atIndex:3];
        [computeEncoder setBuffer:jumpsBuffer offset:0 atIndex:4];
        [computeEncoder setBytes:&atlasParams length: sizeof(atlasParams) atIndex:5];
        
        [computeEncoder setTexture:_characterAtlas atIndex:0];
        
        
        MTLSize threadsPerThreadgroup = MTLSizeMake(16, 16, 1);

        MTLSize threadsPerGrid = MTLSizeMake(maxDistance,
                                             (size+padSize)*numLines,
                                            1);
        
        [computeEncoder dispatchThreads:threadsPerGrid
                  threadsPerThreadgroup:threadsPerThreadgroup];
        
        
        
        [computeEncoder endEncoding];
 
        
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

    }
    
    // Calls the above function with a new calculated size
    void recalculateCharacterAtlas(int height){
        float largestFontSize = 0;
        for (int i = 0; i< _textParams.size(); ++i){
            if(_textParams[i].fontSize>largestFontSize){
                largestFontSize = _textParams[i].fontSize;
            }
        }
        loadTextSize(int(height*largestFontSize*2));
    }
    
    
    // Get an entire text feild as an std::string
    std::string getTextOfFeild(int feild){
        
        int min = _textParams[feild].startsAt;
        int max = _textParams[feild].textEndsAt;
        
        return textToStdString(min+1, max);
    }
    
    // Get the copied text of a text feild as an std::string
    std::string getCopiedText(){
        
        if (_highlightStartIndex == 0) {
            return "";
        }
        
        int min, max;
        if (_cursorIndex > _highlightStartIndex-1) { min = _highlightStartIndex-1; max = _cursorIndex; }
        else { min = _cursorIndex; max = _highlightStartIndex-1; }

        
        return textToStdString(min, max);
    }
    
protected:
    
    
#pragma mark Conversion btwn text formats
    
    // Text (in-engine format) ---> std::string (c++)
    std::string textToStdString(int min, int max){
        
        uint8_t* text = (uint8_t*)_textBuffer.contents;
        
        std::string result = "";
        
        for (int i = min; i<max; ++i){
            
            switch (text[i]) {
                case 1: result += "0"; break;
                case 2: result += "1"; break;
                case 3: result += "2"; break;
                case 4: result += "3"; break;
                case 5: result += "4"; break;
                case 6: result += "5"; break;
                case 7: result += "6"; break;
                case 8: result += "7"; break;
                case 9: result += "8"; break;
                case 10: result += "9"; break;
                case 11: result += "a"; break;
                case 12: result += "b"; break;
                case 13: result += "c"; break;
                case 14: result += "d"; break;
                case 15: result += "e"; break;
                case 16: result += "f"; break;
                case 17: result += "g"; break;
                case 18: result += "h"; break;
                case 19: result += "i"; break;
                case 20: result += "j"; break;
                case 21: result += "k"; break;
                case 22: result += "l"; break;
                case 23: result += "m"; break;
                case 24: result += "n"; break;
                case 25: result += "o"; break;
                case 26: result += "p"; break;
                case 27: result += "q"; break;
                case 28: result += "r"; break;
                case 29: result += "s"; break;
                case 30: result += "t"; break;
                case 31: result += "u"; break;
                case 32: result += "v"; break;
                case 33: result += "w"; break;
                case 34: result += "x"; break;
                case 35: result += "y"; break;
                case 36: result += "z"; break;
                case 37: result += "A"; break;
                case 38: result += "B"; break;
                case 39: result += "C"; break;
                case 40: result += "D"; break;
                case 41: result += "E"; break;
                case 42: result += "F"; break;
                case 43: result += "G"; break;
                case 44: result += "H"; break;
                case 45: result += "I"; break;
                case 46: result += "J"; break;
                case 47: result += "K"; break;
                case 48: result += "L"; break;
                case 49: result += "M"; break;
                case 50: result += "N"; break;
                case 51: result += "O"; break;
                case 52: result += "P"; break;
                case 53: result += "Q"; break;
                case 54: result += "R"; break;
                case 55: result += "S"; break;
                case 56: result += "T"; break;
                case 57: result += "U"; break;
                case 58: result += "V"; break;
                case 59: result += "W"; break;
                case 60: result += "X"; break;
                case 61: result += "Y"; break;
                case 62: result += "Z"; break;
                case 64: result += "_"; break;
                case 65: result += "."; break;
                case 66: result += "`"; break;
                case 67: result += "~"; break;
                case 68: result += "!"; break;
                case 69: result += "@"; break;
                case 70: result += "#"; break;
                case 71: result += "$"; break;
                case 72: result += "%"; break;
                case 73: result += "^"; break;
                case 74: result += "&"; break;
                case 75: result += "*"; break;
                case 76: result += "("; break;
                case 77: result += ")"; break;
                case 78: result += "-"; break;
                case 79: result += "-"; break;
                case 80: result += "+"; break;
                case 81: result += "="; break;
                case 82: result += "{"; break;
                case 83: result += "}"; break;
                case 84: result += "["; break;
                case 85: result += "]"; break;
                case 86: result += "\\"; break;
                case 87: result += "|"; break;
                case 88: result += ":"; break;
                case 89: result += ";"; break;
                case 90: result += "'"; break;
                case 91: result += "\""; break;
                case 92: result += ","; break;
                case 93: result += "<"; break;
                case 94: result += ">"; break;
                case 95: result += "/"; break;
                case 96: result += "?"; break;
                case 98: result += " "; break;
                    
                default: result += "[[ERROR]]"; break;
  
            }
        }
        
        return result;
    }
    
    
    // std::string (c++) ---> Text (in-engine format)
    void stdStringToText(std::vector<uint8_t> *result, std::string input, int group = -1){
        
        int tLength = 0;
        
        for (int i = 0; i<input.length(); ++i){
            
            switch (input[i]) {
                case '0': result->push_back(1); break;
                case '1': result->push_back(2); break;
                case '2': result->push_back(3); break;
                case '3': result->push_back(4); break;
                case '4': result->push_back(5); break;
                case '5': result->push_back(6); break;
                case '6': result->push_back(7); break;
                case '7': result->push_back(8); break;
                case '8': result->push_back(9); break;
                case '9': result->push_back(10); break;
                case 'a': result->push_back(11); break;
                case 'b': result->push_back(12); break;
                case 'c': result->push_back(13); break;
                case 'd': result->push_back(14); break;
                case 'e': result->push_back(15); break;
                case 'f': result->push_back(16); break;
                case 'g': result->push_back(17); break;
                case 'h': result->push_back(18); break;
                case 'i': result->push_back(19); break;
                case 'j': result->push_back(20); break;
                case 'k': result->push_back(21); break;
                case 'l': result->push_back(22); break;
                case 'm': result->push_back(23); break;
                case 'n': result->push_back(24); break;
                case 'o': result->push_back(25); break;
                case 'p': result->push_back(26); break;
                case 'q': result->push_back(27); break;
                case 'r': result->push_back(28); break;
                case 's': result->push_back(29); break;
                case 't': result->push_back(30); break;
                case 'u': result->push_back(31); break;
                case 'v': result->push_back(32); break;
                case 'w': result->push_back(33); break;
                case 'x': result->push_back(34); break;
                case 'y': result->push_back(35); break;
                case 'z': result->push_back(36); break;
                case 'A': result->push_back(37); break;
                case 'B': result->push_back(38); break;
                case 'C': result->push_back(39); break;
                case 'D': result->push_back(40); break;
                case 'E': result->push_back(41); break;
                case 'F': result->push_back(42); break;
                case 'G': result->push_back(43); break;
                case 'H': result->push_back(44); break;
                case 'I': result->push_back(45); break;
                case 'J': result->push_back(46); break;
                case 'K': result->push_back(47); break;
                case 'L': result->push_back(48); break;
                case 'M': result->push_back(49); break;
                case 'N': result->push_back(50); break;
                case 'O': result->push_back(51); break;
                case 'P': result->push_back(52); break;
                case 'Q': result->push_back(53); break;
                case 'R': result->push_back(54); break;
                case 'S': result->push_back(55); break;
                case 'T': result->push_back(56); break;
                case 'U': result->push_back(57); break;
                case 'V': result->push_back(58); break;
                case 'W': result->push_back(59); break;
                case 'X': result->push_back(60); break;
                case 'Y': result->push_back(61); break;
                case 'Z': result->push_back(62); break;
                case '_': result->push_back(64); break;
                case '.': result->push_back(65); break;
                case '`': result->push_back(66); break;
                case '~': result->push_back(67); break;
                case '!': result->push_back(68); break;
                case '@': result->push_back(69); break;
                case '#': result->push_back(70); break;
                case '$': result->push_back(71); break;
                case '%': result->push_back(72); break;
                case '^': result->push_back(73); break;
                case '&': result->push_back(74); break;
                case '*': result->push_back(75); break;
                case '(': result->push_back(76); break;
                case ')': result->push_back(77); break;
                case '-': result->push_back(78); break;
                //case '': result.push_back(79); break;
                case '+': result->push_back(80); break;
                case '=': result->push_back(81); break;
                case '{': result->push_back(82); break;
                case '}': result->push_back(83); break;
                case '[': result->push_back(84); break;
                case ']': result->push_back(85); break;
                case '\\': result->push_back(86); break;
                case '|': result->push_back(87); break;
                case ':': result->push_back(88); break;
                case ';': result->push_back(89); break;
                case '\'': result->push_back(90); break;
                case '\"': result->push_back(91); break;
                case ',': result->push_back(92); break;
                case '<': result->push_back(93); break;
                case '>': result->push_back(94); break;
                case '/': result->push_back(95); break;
                case '?': result->push_back(96); break;
                case ' ': result->push_back(98); break;
                    
                case '\t':
                    result->push_back(0);
                    tLength += 1;
                    break;
                    
                case '\n': result->push_back(99); break;
                    
                default: result->push_back(63); break;
  
            }
            
            if (tLength == 20){ // Detect input feilds
                tLength = 0;
                
                if(group!= -1){
                    while(_fields.size() <group+1){
                        _fields.push_back({});
                    }
                    _fields[group].push_back(i-20+2);
                }
            }
        }

        
    }
    
    
    // Applescript keycode -> std::string
    std::string keyCodeToStdString(uint16_t keyCode){
        switch (keyCode) {
            case 29: return "0";
            case 18: return "1"; // 1 / !
            case 19: return "2"; // 2 / @
            case 20: return "3"; // 3 / #
            case 21: return "4"; // 4 / $
            case 23: return "5"; // 5 / %
            case 22: return "6"; // 6 / ^
            case 26: return "7"; // 7 / &
            case 28: return "8"; // 8 / *
            case 25: return "9"; // 9 / (
                
            case 49: return "Space"; // ' '
                
                // Uppercase and lowercase
            case 0:  return "A"; // a
            case 11: return "B"; // b
            case 8:  return "C"; // c
            case 2:  return "D"; // d
            case 14: return "E"; // e
            case 3:  return "F"; // f
            case 5:  return "G"; // g
            case 4:  return "H"; // h
            case 34: return "I"; // i
            case 38: return "J"; break; // j
            case 40: return "K"; // k
            case 37: return "L"; // l
            case 46: return "M"; // m
            case 45: return "N"; // n
            case 31: return "O"; // o
            case 35: return "P"; // p
            case 12: return "Q"; // q
            case 15: return "R"; // r
            case 1:  return "S"; // s
            case 17: return "T"; // t
            case 32: return "U"; // u
            case 9:  return "V"; // v
            case 13: return "W"; // w
            case 7:  return "X"; // x
            case 16: return "Y"; // y
            case 6:  return "Z"; // z
                
            case 50: return "`"; //  ~ / `
            case 27: return "-"; //  _ / -
            case 24: return "="; //  + / =
            case 33: return "["; //  { / [
            case 30: return "]"; //  } / ]
            case 42: return "\\"; //  | / '\'
            case 41: return ";"; //  : / ;
            case 39: return "\'"; //  " / '
            case 43: return ","; //  < / ,
            case 47: return "."; //  > / .
            case 44: return "/"; //  ? / /
                
            case 36:
            case 76: return "Enter";
            case 48: return "Tab";
                
            case 59: return "Control";
            case 58: return "Left Option";
            case 55: return "Left Command";
                
            case 54: return "Right Command";
            case 61: return "Right Option";
                
                
            case 51: return "Delete";
                
                
                
            case 126: return "Up Arrow";
            case 125: return "Down Arrow";
                
            case 124: return "Right Arrow";
            case 123: return "Left Arrow";
                
            case 56: return "Left Shift";
            case 60: return "Right Shift";
                
            case 53: return "Escape";
            case 200: return "Left Click";
            case 201: return "Right Click";
                
            case 107:
            case 122: return "F1";
                
            case 113:
            case 120: return "F2";
            
            case 160:
            case 99: return "F3";
                
            case 131:
            case 118: return "F4";
                
            case 96: return "F5";
                
            case 97: return "F6";
                
            case 98: return "F7";
                
            case 100: return "F8";
                
            case 101: return "F9";
                
            case 109: return "F10";
                
            case 103: return "F11";
                
            case 111: return "F12";
                
                
                
            default: return std::to_string(keyCode);
        }
    }
    
    
    
#pragma mark Auxilery Text Functions
    
    
    
    // Resize the text buffer and positioning buffer to adapt to an insertion of length 'modificationLength', which may be negative
    void fitTextBuffer(int modificationLength){
            

        uint nextParamStartsAt = ((_textParams.size() - 1 <= _indexOfTextParams) ? _textBufferLength : _textParams[_indexOfTextParams+1].startsAt);
        uint currSpace = nextParamStartsAt - _textParams[_indexOfTextParams].startsAt;
        uint currLengthLetters = _textParams[_indexOfTextParams].textEndsAt - _textParams[_indexOfTextParams].startsAt;
        uint currTotalLengthOther = _textBufferLength - currSpace;
        uint newLengthSingle = alignTo256(modificationLength + currLengthLetters);
        uint newLength = currTotalLengthOther + newLengthSingle;
        
        // Return if the insertion / deletion does not require a reinitiation of the text and positioning buffers
        if (newLength == _textBufferLength){
            return;
        }
        
        uint growBy = newLength - currTotalLengthOther - currSpace;
        
        // Update the textParams in accordence to the new buffer size
        for(int i = _indexOfTextParams+1; i<_textParams.size(); ++i){
            _textParams[i].startsAt += growBy;
            _textParams[i].textEndsAt += growBy;
        }
        
        // The index of where the next text group starts at in the text buffer
        // (This variable contrasts 'nextParamStartsAt', which was this value before the resizing of the buffer)
        uint nextParamNowStartsAt = ((_textParams.size() - 1 <= _indexOfTextParams) ? _textBufferLength : _textParams[_indexOfTextParams+1].startsAt);
                
        
        // Hold a reference to the original data
        id<MTLBuffer> existingTextBuffer = _textBuffer;
        
        // Create a new text buffer of desired length
        _textBuffer = [_device newBufferWithLength:newLength
                                              options:MTLResourceStorageModeShared];
        
        id<MTLBuffer> existingLetterSpacing = _textPositioningBuffer;
        
        _textPositioningBuffer = [_device newBufferWithLength:newLength*sizeof(vector_float2)
                                                options:MTLResourceStorageModeShared];
        
        // Zero-initialize the new buffers
        memset(_textBuffer.contents, 0, newLength);
        memset(_textPositioningBuffer.contents, 0, newLength*sizeof(vector_float2));
        
        
        uint indexOfTextBufferSizeChange = _textParams[_indexOfTextParams].textEndsAt+1;
        
        
        
        // Copy the data that will not be effected by the size change (before it) to the new buffer
        memmove(_textBuffer.contents,
                existingTextBuffer.contents,
                indexOfTextBufferSizeChange);
        memmove(_textPositioningBuffer.contents,
                existingLetterSpacing.contents,
                indexOfTextBufferSizeChange *sizeof(vector_float2));
        
        // Copy the text afterwards into the new buffer, but shifted to allow the new text
        memmove((char*)_textBuffer.contents+nextParamNowStartsAt,
                (char*)existingTextBuffer.contents + (nextParamStartsAt),
                _textBufferLength-nextParamStartsAt);
        memmove((vector_float2*)_textPositioningBuffer.contents+nextParamNowStartsAt, (vector_float2*)existingLetterSpacing.contents + (nextParamStartsAt), (_textBufferLength-nextParamStartsAt)*sizeof(vector_float2));
        
        
        // Update variables
        _textBufferLength = newLength;
        
    }
    
    
    
    // Get the text that has been typed and return a tuple of formatted values that can be passed to the
    // gpu as letters (not keystrokes, which cannot be passed to the gpu as a letter), and 'modifications',
    // which is the list of instructions, all processed at one time (ex. type 5 letters or delete 2 letters,
    // then type two more.)
    std::tuple<std::vector<uint8_t>, std::vector<std::pair<ModificationTypes, int>>> getIncomingText(std::vector<KeyStroke> incomingKeys, std::vector<std::pair<int, std::pair<ModificationTypes, int>>> preformattedModifications, std::vector<int> preformattedText){
                
        // Shortcut to delete a letter if it would not require logic using the modificaiton memory
        bool canDelShort = false;
        
        // Initialized blank formatted return values
        std::vector<uint8_t> incomingKeysFormatted = {};
        std::vector<std::pair<ModificationTypes, int>> modifications = {{ADD, 0}};
        
        // If there are no incoming keys, simply put all the preformatted modificaitons into the modification list
        if (incomingKeys.size()==0) {
            
            incomingKeysFormatted.insert(incomingKeysFormatted.end(), preformattedText.begin(), preformattedText.end());
            
            for (int j = 0; j<preformattedModifications.size(); ++j){
                
                modifications.push_back(preformattedModifications[j].second);
            }
        }
        
        uint8_t lastKey = *((uint8_t*)_textBuffer.contents+_cursorIndex-1);
        float fontSize = _textParams[_indexOfTextParams].fontSize;
        int lastChar = _textParams[_indexOfTextParams].textEndsAt;
        
        simd_float2 currentPos = ((simd_float2*)_textPositioningBuffer.contents)[lastChar];
        int numLinesAlready = -int((currentPos.y-_textParams[_indexOfTextParams].originY) / (fontSize*NewlineDist));
        float distTraveled = currentPos.x-_textParams[_indexOfTextParams].originX;
        
        const float *dividingBuffer = (float*)_characterWidthBuffer.contents;
        
        
        // Convert raw keystrokes into usable values
        for(int i = 0; i<incomingKeys.size(); ++i){
            
            // Add preformatted modifications
            for (int j = 0; j<preformattedModifications.size(); ++j){
                if (preformattedModifications[j].first==i){
                    
                    int start = ((j-3 < 0) ? 0 : preformattedModifications[j-1].second.second);
                    
                    incomingKeysFormatted.insert(incomingKeysFormatted.end(), preformattedText.begin() + start , preformattedText.begin() + start + preformattedModifications[j].second.second);
                    
                    modifications.push_back(preformattedModifications[j].second);
                }
            }
            
            int keyCode = incomingKeys[i].key;
            uint64_t keyFlags = incomingKeys[i].flags;
            bool isShifted = bool(keyFlags & NSEventModifierFlagShift);
            
            
            
            // Create a new modification type if the type of editing changes
            if(keyCode!=51 && keyCode!=126 && keyCode!=125 && keyCode!=124 && keyCode!=123){
                
                // If there is an editing modification group, one can shortcut deletion
                if(modifications.size()==1&&modifications[0].first==ADD) canDelShort = true;
                
                // If the modification is already adding, then increase its magnitude by one.
                // Otherwise, set it to adding
                if(modifications.size()>=1&&modifications[modifications.size()-1].first == ADD){
                    modifications[modifications.size()-1].second += 1;
                } else {
                    modifications.push_back({ADD, 1});
                }
            }
            
            
            // https://eastmanreference.com/complete-list-of-applescript-key-codes
            
            
            // format: isShifted ? [uppercase ex. '{'] : [lowercase ex. '[']
            switch (keyCode) {
                case 29: incomingKeysFormatted.push_back(isShifted ? 77 : 1); break; // 0 / )
                case 18: incomingKeysFormatted.push_back(isShifted ? 68 : 2); break; // 1 / !
                case 19: incomingKeysFormatted.push_back(isShifted ? 69 : 3); break; // 2 / @
                case 20: incomingKeysFormatted.push_back(isShifted ? 70 : 4); break; // 3 / #
                case 21: incomingKeysFormatted.push_back(isShifted ? 71 : 5); break; // 4 / $
                case 23: incomingKeysFormatted.push_back(isShifted ? 72 : 6); break; // 5 / %
                case 22: incomingKeysFormatted.push_back(isShifted ? 73 : 7); break; // 6 / ^
                case 26: incomingKeysFormatted.push_back(isShifted ? 74 : 8); break; // 7 / &
                case 28: incomingKeysFormatted.push_back(isShifted ? 75 : 9); break; // 8 / *
                case 25: incomingKeysFormatted.push_back(isShifted ? 76 : 10); break; // 9 / (
                    
                case 49: incomingKeysFormatted.push_back(98); break; // ' '
                    
                    // Uppercase and lowercase
                case 0:  incomingKeysFormatted.push_back(11 + isShifted * 26); break; // a
                case 11: incomingKeysFormatted.push_back(12 + isShifted * 26); break; // b
                case 8:  incomingKeysFormatted.push_back(13 + isShifted * 26); break; // c
                case 2:  incomingKeysFormatted.push_back(14 + isShifted * 26); break; // d
                case 14: incomingKeysFormatted.push_back(15 + isShifted * 26); break; // e
                case 3:  incomingKeysFormatted.push_back(16 + isShifted * 26); break; // f
                case 5:  incomingKeysFormatted.push_back(17 + isShifted * 26); break; // g
                case 4:  incomingKeysFormatted.push_back(18 + isShifted * 26); break; // h
                case 34: incomingKeysFormatted.push_back(19 + isShifted * 26); break; // i
                case 38: incomingKeysFormatted.push_back(20 + isShifted * 26); break; // j
                case 40: incomingKeysFormatted.push_back(21 + isShifted * 26); break; // k
                case 37: incomingKeysFormatted.push_back(22 + isShifted * 26); break; // l
                case 46: incomingKeysFormatted.push_back(23 + isShifted * 26); break; // m
                case 45: incomingKeysFormatted.push_back(24 + isShifted * 26); break; // n
                case 31: incomingKeysFormatted.push_back(25 + isShifted * 26); break; // o
                case 35: incomingKeysFormatted.push_back(26 + isShifted * 26); break; // p
                case 12: incomingKeysFormatted.push_back(27 + isShifted * 26); break; // q
                case 15: incomingKeysFormatted.push_back(28 + isShifted * 26); break; // r
                case 1:  incomingKeysFormatted.push_back(29 + isShifted * 26); break; // s
                case 17: incomingKeysFormatted.push_back(30 + isShifted * 26); break; // t
                case 32: incomingKeysFormatted.push_back(31 + isShifted * 26); break; // u
                case 9:  incomingKeysFormatted.push_back(32 + isShifted * 26); break; // v
                case 13: incomingKeysFormatted.push_back(33 + isShifted * 26); break; // w
                case 7:  incomingKeysFormatted.push_back(34 + isShifted * 26); break; // x
                case 16: incomingKeysFormatted.push_back(35 + isShifted * 26); break; // y
                case 6:  incomingKeysFormatted.push_back(36 + isShifted * 26); break; // z
                    
                case 50: incomingKeysFormatted.push_back(isShifted ? 67 : 66); break; //  ~ / `
                case 27: incomingKeysFormatted.push_back(isShifted ? 64 : 78); break; //  _ / -
                case 24: incomingKeysFormatted.push_back(isShifted ? 80 : 81); break; //  + / =
                case 33: incomingKeysFormatted.push_back(isShifted ? 82 : 84); break; //  { / [
                case 30: incomingKeysFormatted.push_back(isShifted ? 83 : 85); break; //  } / ]
                case 42: incomingKeysFormatted.push_back(isShifted ? 87 : 86); break; //  | / '\'
                case 41: incomingKeysFormatted.push_back(isShifted ? 88 : 89); break; //  : / ;
                case 39: incomingKeysFormatted.push_back(isShifted ? 91 : 90); break; //  " / '
                case 43: incomingKeysFormatted.push_back(isShifted ? 93 : 92); break; //  < / ,
                case 47: incomingKeysFormatted.push_back(isShifted ? 94 : 65); break; //  > / .
                case 44: incomingKeysFormatted.push_back(isShifted ? 96 : 95); break; //  ? / /
                    
                case 36: // \n
                case 76:
                    if(_textParams[_indexOfTextParams].flags & DISABLE_ENTER_FLAG){
                        modifications.back().second-=1;
                        continue;
                    } else {
                        incomingKeysFormatted.push_back(99);
                    }
                    break; // \n
                    
                    
                case 51: // delete
                    
                    
                    if(incomingKeysFormatted.size()>0 && canDelShort){
                        incomingKeysFormatted.pop_back();
                    } else {
                        
                        // If the modification is already deleting, then increase its magnitude by one.
                        // Otherwise, set it to deleting
                        if(modifications.back().first == DELETE){
                            modifications.back().second += 1;
                        } else {
                            modifications.push_back({DELETE,1});
                        }
                    }
                    
                    break;
                    
                    
                    
                case 126: break; // up arrow
                case 125: break; // down arrow
                    
                case 124: // right arrow
                    
                    if (!isShifted){ // Regular arrow
                        
                        canDelShort = false;
                        
                        // If the modification is already regular arrowing, then increase its magnitude by one.
                        // Otherwise, set it to regular arrowing
                        if(modifications.back().first==ARROW){
                            modifications.back().second += 1;
                        } else {
                            modifications.push_back({ARROW,1});
                        }
                        
                    } else { // Shift arrow
                        canDelShort = false;
                        
                        if(modifications.back().first==SHIFT_ARROW){
                            modifications.back().second += 1;
                        } else {
                            modifications.push_back({SHIFT_ARROW,1});
                        }
                    }
                    
                    
                    break;
                case 123: // left arrow
                    
                    if (!isShifted){ // Regular arrow
                        
                        
                        canDelShort = false;
                        
                        // If the modification is already regular arrowing, then increase its magnitude by one.
                        // Otherwise, set it to regular arrowing
                        if(modifications.back().first==ARROW){
                            modifications.back().second -= 1;
                        } else {
                            modifications.push_back({ARROW,-1});
                        }
                        
                    } else { // Shift Arrow
                        canDelShort = false;
                        
                        if(modifications.back().first==SHIFT_ARROW){
                            modifications.back().second -= 1;
                        } else {
                            modifications.push_back({SHIFT_ARROW,-1});
                        }
                    }
                    
                    break;
                    
                    
                default: break; //incomingKeysFormatted.push_back(63); break; // Error char
            }
            
            
            // Apply restrictions to text feilds (i.e. max length, or an integer only feild (like setting an FOV))
            if(incomingKeysFormatted.size() >= 1) {
                
                // If the text feilds only allows integers, if the back key is not an int, pop it
                if ((_textParams[_indexOfTextParams].flags & INTEGER_ONLY_FLAG)
                        && (incomingKeysFormatted.back() > 10 || incomingKeysFormatted.back() == 0)) {
                    
                    incomingKeysFormatted.pop_back();
                    modifications.back().second-=1;
                    continue;
                
                // If the text feild only allows integers, check if the key is not int AND not a '.', and pop it
                } else if ((_textParams[_indexOfTextParams].flags & FLOAT_ONLY_FLAG)
                           && (incomingKeysFormatted.back() > 10 || incomingKeysFormatted.back() == 0)
                           && incomingKeysFormatted.back() != 65) {
                    
                    incomingKeysFormatted.pop_back();
                    modifications.back().second-=1;
                    continue;
                }
                
                // Apply max lengths:
                
                // If the max length is in lines, calculate an approximation of the length of the letter, add it
                // to the current length, and see if that surpasses the number of lines. If it is, pop the letter.
                // This value does not need to be exact (batched inputs may not be accuratley represented) as
                // inptus that have a max length in lines are mostly customization, not anything important (such
                // as naming something)
                if (_textParams[_indexOfTextParams].flags & MAX_LENGTH_LINES_FLAG){
                    
                    // Enter keys add one to the number of lines
                    if(incomingKeysFormatted.back()==99) {
                        numLinesAlready+=1;
                    }
                    
                    // Kerning
                    float kernLength = 0.0;
                    uint16_t hashableIndex = (lastChar<<8)|incomingKeysFormatted.back();
                    if(kernsSingle.contains(hashableIndex)){
                        kernLength = kernsSingle.at(hashableIndex);
                    }
                    
                    // Calculate the total width of the character (width - kern) * size
                    float letterWidth = (abs(dividingBuffer[incomingKeysFormatted.back()]) - kernLength) * fontSize;
                    
                    // If the distance surpassed is too many lines, than pop the letter that overflowed it
                    if ((distTraveled + letterWidth + fontSize * 0.06 * (letterWidth!=0))/_textParams[_indexOfTextParams].lineWrappingLength + numLinesAlready >= _textParams[_indexOfTextParams].maxLength) {
                        
                        incomingKeysFormatted.pop_back();
                        modifications.back().second-=1;
                        continue;
                        
                    } else {
                        
                        // Otherwise, assume the typed letter to be the new baseline and continue
                        lastKey = incomingKeysFormatted.back();
                        distTraveled += letterWidth + fontSize * 0.06 * (letterWidth!=0);
                    }
                    
                    
                    
                } else {
                    
                    // If the max length is expressed as a number of chars: calculate the number of characters,
                    // and pop the typed letter(s) if they surpass the max number of chars
                    
                    int numChars = int(incomingKeysFormatted.size())
                    - _textParams[_indexOfTextParams].startsAt
                    + _textParams[_indexOfTextParams].textEndsAt
                    - ((_highlightStartIndex == 0) ? 0 : _cursorIndex - (_highlightStartIndex - 1));
                    
                    if (numChars > _textParams[_indexOfTextParams].maxLength + 2) {
                        
                        incomingKeysFormatted.pop_back();
                        modifications.back().second-=1;
                        continue;
                        
                    }
                    
                }
                
            }
        }

        // Return both the keys typed and the modifications with said keys
        return std::make_tuple(incomingKeysFormatted, modifications);
    }
    
    
    
#pragma mark Text Editing Pathways
    
    
    // If the text edit is only additive, this function may be called. If the edit was AAAABBBBCCCC, where
    // AAAA is original text before the insertion, BBBB is the insertion, and CCCC is the original text
    // after the insertion, then this function first takes AAAACCCC, moves the C region to make space, making
    // AAAA____CCCC, then adds in the C region, returning AAAABBBBCCCC
    int additiveTextEdit(std::vector<uint8_t> incomingKeysFormatted){
            
        int length = int(incomingKeysFormatted.size());
        
        // Resize the buffers if necessary
        fitTextBuffer(length);
        
        // Make space for the incoming text
        memcpy((uint8_t*)_textBuffer.contents+_cursorIndex+length,
               (uint8_t*)_textBuffer.contents+_cursorIndex,
               _textParams[_indexOfTextParams].textEndsAt-_cursorIndex+1);
        
        // Copy incoming text into buffer
        memcpy((uint8_t*)_textBuffer.contents+_cursorIndex,
               incomingKeysFormatted.data(),
               length);
        
        _cursorIndex+=length;

        return length;
        
    }
    
    // If the text edit is only distructive, this function may be called. If the edit was AAAABBBBCCCC, where
    // AAAA is original text before the deletion, BBBB is the deleted text, and CCCC is the text after the
    // deletion, then this function first takes AAAABBBBCCCC, moves the C region into the B region, making
    // AAAACCCCCCCC, then zeroes the duplicated C region, returning AAAACCCC
    int deletionTextEdit(std::vector<uint8_t> incomingKeysFormatted, int distance){
        
        int start = _textParams[_indexOfTextParams].startsAt ;
        
        // Don't let the user delete into another text group
        distance = MIN(MIN(_cursorIndex-start, distance),_textParams.back().textEndsAt-start);
        
        if (distance <= 0) { return 0; }
        
        // Move the data following the deletion over
        memmove((uint8_t*)_textBuffer.contents+_cursorIndex-distance,
                (uint8_t*)_textBuffer.contents+_cursorIndex,
                _textParams[_indexOfTextParams].textEndsAt-_cursorIndex-distance+2);
        
        // Clear the overlapping region
        memset((uint8_t*)_textBuffer.contents+_textParams[_indexOfTextParams].textEndsAt, 0, distance);
        
        // Resize the text buffer (this needs to be done after moving the data so that nothing is truncated)
        fitTextBuffer(-distance);
        
        _cursorIndex-=distance;
        
        return distance;
        
    }
    
    // If the text edit is a mix of several types, this function may be used. It treats the contents of a singular
    // text group as multiple sup-text-groups. The cursor is always at the end of one, cutting one into two if
    // necissary. The text groups could either be original or inserted text. After all groups are arranged
    // as necessary, few memory operations are used to minipulate the source original and inputted texts into one.
    std::tuple<int, int> handleComplexSteppedProcesses(std::vector<uint8_t> incomingKeysFormatted, std::vector<std::pair<ModificationTypes, int>> modifications){
        
        // Initiate the text groups as (0-cursor), (cursor-end)
        std::vector<textOrderer> textGroups = {
            {TextOrderer::ORIGINAL, _textParams[_indexOfTextParams].startsAt, _cursorIndex-1},
            {TextOrderer::ORIGINAL, _cursorIndex, _textParams[_indexOfTextParams].textEndsAt},
        };
        
        // If the last text group is empty, pop it
        if(_cursorIndex==_textParams[_indexOfTextParams].textEndsAt+1){
            textGroups.pop_back();
        }
        
        int localCursorPosition = 0; // The text group the cursor is at
        int typingCursorPosition = 0; // Which characters the cursor has typed from the inputted text as an index
        int numDeleted = 0;
        
        
        // Loop over each modification and applt it
        for(int i = 0; i<modifications.size(); ++i){
            
            ModificationTypes process = modifications[i].first;
            int magnitude = modifications[i].second;

            // If the modification does nothing, do nothing
            if(magnitude==0){
                continue;
            }
            
            // If the process is shift arrows, set the cursor values positions and default to the
            // regular arrow protocol
            if (process == SHIFT_ARROW){
                if (_highlightStartIndex == 0) {
                    _highlightStartIndex = MIN(MAX(_cursorIndex + 1, _textParams[_indexOfTextParams].startsAt), _textParams[_indexOfTextParams].textEndsAt);
                }
                process = ARROW;
                
            } else if (_highlightStartIndex != 0){ // Handle logic relating to typing while text is highlighted
                if (process != ARROW){
                    
                    // If the process is to delete, don't actually run the original delete code, instead
                    // allow the below logic to clear the highlighted regions
                    if (process == DELETE) {
                        if(magnitude==1){
                            modifications[i].first = SKIP;
                            process = SKIP;
                        } else {
                            magnitude -= 1; // Unless multiple delete keys were used
                        }
                    }
                    
                    // If the '_highlightStartIndex' is after '_cursorIndex', swap them using an arrow call
                    if (_highlightStartIndex-1>_cursorIndex){
                        
                        i-=1;
                        process = ARROW;
                        magnitude = _highlightStartIndex-1-_cursorIndex;
                        _highlightStartIndex = MIN(MAX(_cursorIndex + 1, _textParams[_indexOfTextParams].startsAt), _textParams[_indexOfTextParams].textEndsAt);
                        
                    } else {
                        
                        // If the '_highlightStartIndex' and '_cursorIndex' are oriented correctly, and a
                        // non-arrow modification occurs, delete the highlighted text
                        i-=1;
                        process = DELETE;
                        magnitude = _cursorIndex - (_highlightStartIndex -1);
                        
                        _highlightStartIndex = 0;
                        
                    }
                   
                } else { // If the process is simply an arrow, disregard that text was highlighted
                    _highlightStartIndex = 0;
                }
                
            }

            
            // Skip to the next instance if the mode of skip is called.
            if (process == SKIP) { continue; }
            
            
            // Process the number of consecutive deletions
            if(process == DELETE) {
                
                // If one is deleting a negative amount of letters, skip
                if (magnitude < 0) { continue; }
                
                
                int start = _textParams[_indexOfTextParams].startsAt + 1;
                magnitude = MIN(MIN(_cursorIndex-start, magnitude),_textBufferLength-start);
                
                // Subtract the remaining distance (cumulativeDistanceDel) from each text group until a text group
                // is not destroyed by the subtraction in length per
                int cumulativeDistanceDel = magnitude;
                while(true){
                    textOrderer *group = &textGroups[localCursorPosition];
                    
                    group->endOfGroup-=cumulativeDistanceDel;
                    
                    if(group->endOfGroup<group->startOfGroup){
                        cumulativeDistanceDel = group->startOfGroup-group->endOfGroup -1;
                        textGroups.erase(textGroups.begin()+localCursorPosition);
                        
                        localCursorPosition-=1;
                        
                    } else {
                        break;
                    }
                    
                }
                
                _cursorIndex -= magnitude;
                numDeleted += magnitude;
                
                
            // Process the number of consecutive arrows
            } else if (process == ARROW){
                
                // If the arrow does not push into another text group (not a subgroup), continue
                if((magnitude<0 && _cursorIndex==0) || (magnitude>0 && _cursorIndex==_textBufferLength)){
                    continue;
                }
                
                
                int start = _textParams[_indexOfTextParams].startsAt + 1;
                int end = _textParams[_indexOfTextParams].textEndsAt-1;
                _cursorIndex=MAX(MIN(_cursorIndex+magnitude, end), start); // cap the arrow movement
                
                
                // Check to see if any text subgroup contains the arrow movement
                
                int distance = _textParams[_indexOfTextParams].startsAt;
                bool found = false; // A boolean value checking if a text subgroup was ever found
                
                for(int m = 0; m<textGroups.size(); ++m){
                    
                    // Update the travled distance
                    distance += textGroups[m].endOfGroup-textGroups[m].startOfGroup+1;
                    
                    // If the text subgroup perfectly matches the new cursor index, just move the cursor to
                    // said subgroup
                    if(_cursorIndex==distance&&!found){
                        localCursorPosition=m;
                        found = true; break;
                        
                    // If the text subgroup contains the new cursor index but does NOT match it exactly,
                    // split it in two with the cursor as the subdivider
                    } else if(_cursorIndex<distance&&!found){
                        
                        textGroups.insert(textGroups.begin()+m+1, {textGroups[m].source, _cursorIndex, textGroups[m].endOfGroup});
                        textGroups[m].endOfGroup=_cursorIndex-1;
                        
                        localCursorPosition = m;
                        found = true; break;
                    }
                }
                
                // If a text group was never found that contains the new cursor position, (meaning that it
                // was past the end of the text subgroups), put the cursor at the furthest possible point
                if(!found){
                    localCursorPosition = int(textGroups.size()) - 1;
                }
                

            // Process the number of consecutive text inputs
            } else if (process == ADD) {
                
                // If the text group is addition and ends at where the insertion starts, simply add on to that
                // same addition text group
                if(textGroups[localCursorPosition].source == TextOrderer::ADDITION && textGroups[localCursorPosition].endOfGroup==typingCursorPosition && magnitude >= 0){
                    
                    textGroups[localCursorPosition].endOfGroup+=magnitude;
                    
                // Otherwise, add a new text subgroup that is the addition
                } else if (magnitude >= 0){
                    localCursorPosition+=1;
                    textGroups.insert(textGroups.begin()+localCursorPosition, {TextOrderer::ADDITION, typingCursorPosition, typingCursorPosition+magnitude-1});
                }
                
                
                typingCursorPosition+=magnitude;
                _cursorIndex+=magnitude;
                numDeleted -= magnitude;
            }
            
        }
        
        // If more text was added then deleted, resize the text buffer
        if(numDeleted < 0) {
            fitTextBuffer(-numDeleted);
        }
        
        // If the last text group is invalid, pop it
        if(textGroups.back().startOfGroup>textGroups.back().endOfGroup){
            textGroups.pop_back();
        }
        
        // Find the total distance traveled by the entire text group
        int distanceTyped = _textParams[_indexOfTextParams].textEndsAt + 1 - numDeleted;

        // Move each block of original data to the new buffer. Do this backwards so that text is not overwritten
        for(int k = int(textGroups.size()-1); k>=0; --k){
            
            distanceTyped -= textGroups[k].endOfGroup-textGroups[k].startOfGroup+1;
            
            if(textGroups[k].source == TextOrderer::ORIGINAL){
                
                memmove((uint8_t*)_textBuffer.contents+distanceTyped,
                        (uint8_t*)_textBuffer.contents+textGroups[k].startOfGroup,
                        textGroups[k].endOfGroup-textGroups[k].startOfGroup+1);
            }
        }
        
        
        distanceTyped = _textParams[_indexOfTextParams].startsAt;
        
        // Move each block of added text to the new buffer (after original text so that no original text
        // is overwritten)
        if (incomingKeysFormatted.size()>0){
            
            for(int k = 0; k<textGroups.size(); ++k){
                
                if(textGroups[k].source == TextOrderer::ADDITION){
                    
                    memcpy((uint8_t*)_textBuffer.contents+distanceTyped, (uint8_t*)incomingKeysFormatted.data()+textGroups[k].startOfGroup,  textGroups[k].endOfGroup-textGroups[k].startOfGroup+1);
                }
                distanceTyped += textGroups[k].endOfGroup-textGroups[k].startOfGroup+1;
            }
        }
        
        // Zero out / resize the buffer if any text was deleted
        if(numDeleted > 0) {
            fitTextBuffer(-numDeleted);
            memset((uint8_t*)_textBuffer.contents+_textParams[_indexOfTextParams].textEndsAt-numDeleted, 0, MAX(numDeleted,0));
        }
        
        return std::make_tuple(-numDeleted, textGroups[0].endOfGroup-textGroups[0].startOfGroup+2);
    }
    
    
    
    
#pragma mark Text Editing
    
    
    // The function called to update typing
    void stepTyping(){
        
        // Don't do anything if there is nothing to do
        if ([_mtkView isTextEmpty]) return;
        
        // Reset the cursor blink
        _cursorBlink = 0;
        
        // If the highlight is of length zero, simply turn off the highlight
        if(_highlightStartIndex - 1 == _cursorIndex) {
            _highlightStartIndex = 0;
        }
        
        
        // Get the list of modifications, and the text that is added (may be of length 0 if modifications
        // are only deletion)
        
        std::vector<std::pair<int, std::pair<ModificationTypes, int>>> preModifications;
        std::vector<int> preText;
        std::vector<KeyStroke> text;
        
        // Get these values from the mtkView. As obj-c does not allow c++ types, cast a pointer of the value
        // to a solid reference to a local value of the correct type.
        
        auto result = [_mtkView getText];
        if (result) { text = *static_cast<std::vector<KeyStroke>*>(result); }
        else { text = {}; }
        
        result = [_mtkView getPreformattedEffectMap];
        if (result) { preModifications = *static_cast<std::vector<std::pair<int, std::pair<ModificationTypes, int>>>*>(result); }
        else { preModifications = {}; }
        
        result = [_mtkView getPreformattedEffectMapText];
        if (result) { preText = *static_cast<std::vector<int>*>(result); }
        else { preText = {}; }
        
        [_mtkView clearText];
        
        auto keys = getIncomingText(text, preModifications, preText);
        
        std::vector<uint8_t> incomingKeysFormatted = std::get<0>(keys);
        std::vector<std::pair<ModificationTypes, int>> modifications = std::get<1>(keys);

        
        int indexModificationOccurred = (_highlightStartIndex == 0) ? _cursorIndex : MIN(_cursorIndex, _highlightStartIndex-1);

        // Recalculate which text param the modification occured in
        _indexOfTextParams = 0;
        while(_indexOfTextParams+1<_textParams.size() && _textParams[_indexOfTextParams+1].startsAt<indexModificationOccurred) {
            ++_indexOfTextParams;
        }
        
        if (_textParams[_indexOfTextParams].flags & DISABLE_EDIT_FLAG) { return; }
        
        
        int lengthOfModification;
        
        
        if(modifications.size()==1 && _highlightStartIndex == 0 && modifications[0].second>=0){ // Addition
            lengthOfModification = additiveTextEdit(incomingKeysFormatted);
            indexModificationOccurred += lengthOfModification;
            
        } else if (modifications.size()==4 && modifications[0].second==0&&modifications[1].first==DELETE && _highlightStartIndex == 0) { // Deletion
            
            lengthOfModification = -deletionTextEdit(incomingKeysFormatted,modifications[1].second);
            indexModificationOccurred += lengthOfModification;
           
        } else { // more complicated handling
            auto result = handleComplexSteppedProcesses(incomingKeysFormatted, modifications);
            lengthOfModification = std::get<0>(result);
            indexModificationOccurred = std::get<1>(result);
        }
        
        // Debug -- print the entire buffer
        //for (int i = 0; i<512; ++i){ if(i==256){ std::cout << "\n"; } std::cout << (int)(*((uint8_t*)_textBuffer.contents + i)) << " "; } std::cout << "\n\n--\n\n";
        
        
        // Update text params (expand the current param to include the current param + if the )
        _textParams[_indexOfTextParams].textEndsAt += lengthOfModification;
        
        
        stepSpacing(indexModificationOccurred);

    }
    
    
#pragma mark Spacing Logic
    
    
    // Update the spacing for the text (each letter's position on the screen is pre-determined and static,
    // stored inside '_textPositioningBuffer')
    // This algorithm starts at the start of the word being edited and runs through the entire text group
    // until it reaches the end of it. It starts at the start of the word so that if a word is lengthened
    // to the point that it needs to be completely wrapped to the next line, it may be.
    void stepSpacing(int indexModificationOccurred, int groupOverride = -1){
        
        // Allow changing which param group is being modified so that one can create spacing for text groups,
        // even if the cursor is not actively on it
        int group = _indexOfTextParams;
        if (groupOverride != -1){
            group = groupOverride;
        }
        
        if(indexModificationOccurred==-1)
            return;
        
        
        vector_float2 *spacingBuffer = (vector_float2*)_textPositioningBuffer.contents;
        
        const uint8_t *textBuffer = (uint8_t*)_textBuffer.contents;
        const float *dividingBuffer = (float*)_characterWidthBuffer.contents;
        
        float lineWrappingLength = _textParams[group].lineWrappingLength;
        float originX = _textParams[group].originX;
        float originY = _textParams[group].originY;
        float fontSize = _textParams[group].fontSize;

        
        // Find the number of letters in the current word
        int numInTypeWord = 0;
        for(int i = indexModificationOccurred-1; i>0; i--){
            
            const uint8_t letter = textBuffer[i];
            
            if(letter==0||letter == 98) {
                break;
            } else {
                numInTypeWord += 1;
            }
        }
        
        // Start spacing calculation from the start of the word, and end at the end of the text param
        int reiterateStart = MAX(indexModificationOccurred-numInTypeWord-1,_textParams[group].startsAt);
        int endLoop = _textParams[group].textEndsAt + 1;
        
        // Find where the first letter is located
        vector_float2 progress;
        if(reiterateStart==_textParams[group].startsAt) { // If its the first leter, its located at the origin
            progress.x = originX; progress.y = originY;
        } else { // If its not the first letter, grab the location of the last letter, and that is where you start
            progress = spacingBuffer[reiterateStart];
        }
        
        
        int numConsecInWord = 0;
        bool isOnNewline = true; // Dont allow word wrapping to the next line as the first word
        bool alreadyLookedBack = false; // Stop infinite loops of looking back. If a word jumps to the next line
        // out of length, and then is even longer then that, this boolean values stops infinite loops, ex:
        // |apple abcd...| -> |apple        | -> |apple        |   This last step      |apple        |
        //                    |abcdefg...   |    |abcdefghijklm|   would otherwise     |             |
        //                                       |nopqrstuvw...|   cause an infinite   |     ...     |
        //                                                         loop, being:        |             |
        
        // Loop through each letter in the needed range, and recalculate the positioning of them
        for(int i = reiterateStart; i<endLoop+1; ++i){
            
            const uint8_t letter = (i!=endLoop) ? textBuffer[i] : 0;
            float widthOfCharacter = abs(fontSize * dividingBuffer[letter]);
            
            if(letter==0) { // If char is 0 (background spacing / seperation char)
                spacingBuffer[i] = progress;
                numConsecInWord = 0;
                alreadyLookedBack = false;
                isOnNewline = true;
                continue;
                
            } else if (letter == 98) { // If char is ' '
                numConsecInWord = 0;
                alreadyLookedBack = false;
                isOnNewline = false;
                
            } else if (letter == 99) { // If char is a newline
                numConsecInWord = 0;
                
                progress.x = originX;
                progress.y -= fontSize * NewlineDist;
                
                alreadyLookedBack = false;
                isOnNewline = true;
                continue;
                
            } else { // If char is anything else, it means that it is part of a word/number
                numConsecInWord += 1;
            }
            
            
            // The amount of space subtracted up by kerning
            float kernLength = 0;
            
            // Kerning
            uint16_t hashableIndex = (textBuffer[i-1]<<8)|letter;
            if(kernsSingle.contains(hashableIndex)){
                kernLength = kernsSingle.at(hashableIndex);
            }
            
            // Line wrapping
            if(kernLength + lineWrappingLength - ((progress.x + widthOfCharacter) - originX) >=0) {
                progress.x -= kernLength * fontSize; // If no line wraps, just apply the kern
                
            } else { // If there is a line wrap
                
                // If the word is of length zero (and as such doesnt need to be rewound to apply
                // the newline to the full word) or the word has already been rewound (look above),
                // apply a newline without applying it to the prior characters in the word
                if(numConsecInWord==1 || alreadyLookedBack){
                    progress.x = originX;
                    progress.y -= fontSize * NewlineDist;
                    
                    isOnNewline = true;
                    
                // Otherwise, go back to the first letter of the word, put it to the next line, and recalculate
                } else {
                    
                    i-=numConsecInWord;
                    
                    numConsecInWord=0;
                    
                    alreadyLookedBack = true;
                    
                    progress.x = originX;
                    
                    if(!isOnNewline) {
                        progress.y -= fontSize * NewlineDist;
                        isOnNewline = true;
                    }
                    
                    continue;
                }
                
                
            }
           
            
            spacingBuffer[i].x = progress.x;
            
            // Shift letters down if the size is negative (ex. j/g)
            spacingBuffer[i].y = progress.y - 0.1666666666 * (dividingBuffer[letter]<0) * fontSize;
            
            // Add the letter width and space inbetween letter width
            progress.x += widthOfCharacter + fontSize * 0.06;
            
        }
    }
    
    
    

    

    
#pragma mark -
#pragma mark Drawing functions
    

    
    
    
    
    // Draw all text on screen by instancing quads, each having one letter, to generate all of the text
    void drawText (id<MTLRenderCommandEncoder> renderEncoder, uint8_t currentBufferIndex)
    {

        [renderEncoder setVertexBuffer:_textBuffer
                                offset:0
                               atIndex:0];
        
        [renderEncoder setVertexBuffer:_letterTexCoords
                                offset:0
                               atIndex:1];
        
        [renderEncoder setVertexBuffer:_textPositioningBuffer
                                offset:0
                               atIndex:2];
        
        [renderEncoder setVertexBuffer:_characterWidthBuffer
                                offset:0
                               atIndex:3];
        
        [renderEncoder setVertexBuffer:_textParamsBuffer
                                offset:0
                               atIndex:4];
        
        [renderEncoder setVertexBytes:&_letterDims
                                 length:sizeof(simd_float2)
                                atIndex:5];
        
        [renderEncoder setVertexBytes:&_aspect
                                 length:sizeof(float)
                                atIndex:6];
        
        
        int sizeOfTextParams = int(_textParams.size());
        [renderEncoder setVertexBytes:&sizeOfTextParams
                                 length:sizeof(int)
                                atIndex:7];
        
        [renderEncoder setVertexBytes:_scrollOffset
                                length:sizeof(float)
                               atIndex:8];
        
        
        [renderEncoder setFragmentTexture:_characterAtlas
                               atIndex:0];
        
        
        
        [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                  indexCount:6
                                   indexType:MTLIndexTypeUInt16
                                 indexBuffer:_squareIndexBuffer
                           indexBufferOffset:0
                               instanceCount:_textBufferLength];

    }
    
    
    
    
    void drawCursor (id<MTLRenderCommandEncoder> renderEncoder) {
        
        // Advance the cursor blinking animation
        _cursorBlink += 0.08;
        

        // If a region is highlighted, run the highlight shader logic
        if(_highlightStartIndex-1 != _cursorIndex && _highlightStartIndex!=0 ){
            
            [renderEncoder setRenderPipelineState:_highlightTextPipelineState];
            
            
            // Calculate the start, and end of the highlight in coordinates
            
            simd_float2* spacing = (simd_float2*)_textPositioningBuffer.contents;
            
            std::vector<simd_float2> rects; // Each highlighted line to be drawn
            
            int startIndex = (_cursorIndex<_highlightStartIndex-1) ? _cursorIndex : _highlightStartIndex-1;
            int endIndex = (_cursorIndex>_highlightStartIndex-1) ? _cursorIndex : _highlightStartIndex-1;
                
            simd_float2 start = spacing[startIndex]; // Coordinates
            simd_float2 end = spacing[endIndex];
            
            float fontSize = _textParams[_indexOfTextParams].fontSize;
            float originX = _textParams[_indexOfTextParams].originX;
            float originY = _textParams[_indexOfTextParams].originY;
            
            
            start.y = int((start.y-originY) / (fontSize*NewlineDist)) * (fontSize*NewlineDist) + originY;
            end.y = int((end.y-originY) / (fontSize*NewlineDist)) * (fontSize*NewlineDist) + originY;
            
            start.y -= 0.225 * fontSize;
            end.y -= 0.225 * fontSize;
            
            
            simd_float2 lineStart = start;
            
            // Add a line for each line inbetween the first and last
            while (true) {
                
                simd_float2 lineEnd;
                bool isLastLine = lineStart.y - fontSize * NewlineDist< end.y-0.01;
                if (isLastLine) {
                    lineEnd = end;
                    lineEnd.y += _textParams[_indexOfTextParams].fontSize * 1.3;
                } else {
                    lineEnd = simd_make_float2(originX+_textParams[_indexOfTextParams].lineWrappingLength, lineStart.y + _textParams[_indexOfTextParams].fontSize * 1.3);
                }
                
                
                rects.push_back(simd_make_float2(lineStart.x,lineEnd.y));
                rects.push_back(lineEnd);
                rects.push_back(lineStart);
                rects.push_back(simd_make_float2(lineEnd.x,lineStart.y));
                
                lineStart.x = originX;
                lineStart.y -= fontSize * NewlineDist;
                
                if (isLastLine) { break; }
            }
            
            // Draw each hilighting rect (inverted colors)
            
            [renderEncoder setVertexBytes:rects.data()
                                   length:rects.size()*sizeof(simd_float2)
                                  atIndex:0];
            
            [renderEncoder setVertexBytes:&_aspect
                                   length:sizeof(_aspect)
                                  atIndex:1];
            
            
            [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                      indexCount:6
                                       indexType:MTLIndexTypeUInt16
                                     indexBuffer:_squareIndexBuffer
                               indexBufferOffset:0
                                   instanceCount:rects.size()/4];
            
            
        // If a region is not highlighted, draw the cursor
        } else {

            if (_textParams[_indexOfTextParams].flags & DISABLE_EDIT_FLAG) { return; }
            
            
            // Only give the necessary bytes to the shader to optimise memory transfer
            const int charIndexOfCursorGlyph = 97;
            
            [renderEncoder setRenderPipelineState:_cursorPipelineState];
            
            [renderEncoder setVertexBuffer:_letterTexCoords
                                    offset:charIndexOfCursorGlyph*sizeof(simd_float2)
                                   atIndex:0];
            
            [renderEncoder setVertexBuffer:_textPositioningBuffer
                                    offset:_cursorIndex*sizeof(simd_float2)
                                   atIndex:1];
            
            [renderEncoder setVertexBuffer:_characterWidthBuffer
                                    offset:charIndexOfCursorGlyph*sizeof(float)
                                   atIndex:2];
            
            [renderEncoder setVertexBuffer:_textParamsBuffer
                                    offset:_indexOfTextParams*sizeof(truncatedTextPerameters)
                                   atIndex:3];
            
            [renderEncoder setVertexBytes:&_letterDims
                                     length:sizeof(simd_float2)
                                    atIndex:4];
            
            [renderEncoder setVertexBytes:&_textParams[_indexOfTextParams].originY
                                   length:sizeof(float)
                                  atIndex:5];
            
            [renderEncoder setVertexBytes:&_aspect
                                     length:sizeof(float)
                                    atIndex:6];
            
            
            
            [renderEncoder setFragmentTexture:_characterAtlas
                                      atIndex:0];
            
            [renderEncoder setFragmentBytes:&_cursorBlink
                                     length:sizeof(_cursorBlink)
                                    atIndex:0];
            
            
            [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                      indexCount:6
                                       indexType:MTLIndexTypeUInt16
                                     indexBuffer:_squareIndexBuffer
                               indexBufferOffset:0
                                   instanceCount:1];
        }
    }
    
    
    
    
    // A function used to draw the UI from one call (overrided)
    virtual void drawUI (id<MTLRenderCommandEncoder> renderEncoder, uint8_t currentBufferIndex) {
        std::terminate();
    }
    
    
};

