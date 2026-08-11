
class PlayerController {
public:
    
    PlayerController (KeyboardMTKView* mtkView, matrix_float4x4 projectionMatrix) : _mtkView(mtkView), _projectionMatrix(projectionMatrix) {
        eyePos =  {0.f, 2.f, -12.f};
        _eyeTarget = {1.571f, -0.25f, 1.f};
        cameraDragMode = false;
    }
    
    
    void lockCameraUnlockMouse(){
        [NSCursor unhide];
        CGAssociateMouseAndMouseCursorPosition(true);
        
        cameraDragMode = false;
        [_mtkView clearDeltas];
    }
    
    void unlockCameraLockMouse(){
        [NSCursor hide];
        CGAssociateMouseAndMouseCursorPosition(false);
        
        cameraDragMode = true;
        [_mtkView clearDeltas];
    }
    
    
    void tickController (CameraParams* buffer) {

        // Handle camera drag mode
        if (cameraDragMode) {
            
            _eyeTarget.y -= [_mtkView getMouseDeltaY] / 500.0; // Fix to remove magic numbers
            _eyeTarget.x -= [_mtkView getMouseDeltaX] / 500.0;
            [_mtkView clearDeltas];
            
            _eyeTarget.x = fmodf(_eyeTarget.x, 6.283185f); // 2 * PI for better wrapping
            _eyeTarget.y = fmaxf(1.571f, fminf(_eyeTarget.y, 4.710f)); // Clamping vertical rotation
            
            CGPoint center;
            NSSize windowFrame = _mtkView.window.frame.size;
            
            // Handle the title bar being included inside of the window frame bounds
            if( (_mtkView.window.styleMask & NSWindowStyleMaskFullScreen) != NSWindowStyleMaskFullScreen )
                windowFrame.height -= 30;
            
            center.x = windowFrame.width / 2.0;
            center.y = windowFrame.height / 2.0;
            
            center.x += _mtkView.window.frame.origin.x;
            center.y =  _mtkView.window.screen.frame.size.height - _mtkView.window.frame.origin.y - center.y;
            
            CGWarpMouseCursorPosition(center);
        }

        float factorA = sin(_eyeTarget.x);
        float factorB = cos(_eyeTarget.x);
        
        const vector_float3 keyDeltas[] = {
            {0.15f * factorB, 0.f, 0.15f * factorA},  // Key 0: Move forward
            
            {-0.15f * factorA, 0.f, 0.15f * factorB}, // Key 1: Move left
            {-0.15f * factorB, 0.f, -0.15f * factorA}, // Key 2: Move backward
            {0.15f * factorA, 0.f, -0.15f * factorB},  // Key 3: Move right
            
            {0.f, 0.15f, 0.f},  // Key 4: Move up
            {0.f, -0.15f, 0.f}  // Key 5: Move down
        };
 

        for (int i = 0; i < sizeof(keyDeltas) / sizeof(keyDeltas[0]); ++i) {
            if ([_mtkView getKey: i]) {
                eyePos += keyDeltas[i];
            }
        }
        
        // Calculate eye target and up vector
        vector_float3 eyeTarget = eyePos + (vector_float3){cosf(_eyeTarget.x), tanf(_eyeTarget.y), sinf(_eyeTarget.x)};
        vector_float3 eyeUp = {0.f, 1.f, 0.f};
        
        //NSLog(@"%f %f %f", _eyeTarget.x, _eyeTarget.y, _eyeTarget.z);
        
        matrix_float4x4 viewMatrix = matrix_look_at_left_hand(eyePos, eyeTarget, eyeUp);
        
        buffer->viewProjectionMatrix = matrix_multiply(_projectionMatrix, viewMatrix);
        buffer->cameraPos = eyePos;
    }
    
    void setProjectionMatrix (matrix_float4x4 projectionMatrix) {
        _projectionMatrix = projectionMatrix;
    }
    
    simd_float3* getPos(){
        return &eyePos;
    }
    
private:
    
    KeyboardMTKView* _mtkView;
    matrix_float4x4 _projectionMatrix;
    
    vector_float3 eyePos;
    vector_float3 _eyeTarget;
    bool cameraDragMode;
    
};
