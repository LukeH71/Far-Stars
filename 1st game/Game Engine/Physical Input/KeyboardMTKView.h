


#import <MetalKit/MTKView.h>
#import <Foundation/Foundation.h>

#import "KeyBinds.h"


typedef struct {
    uint16_t key;
    uint64_t flags;
} KeyStroke;


@interface KeyboardMTKView : MTKView <NSUserInterfaceValidations>

- (void)initKeys:(uint8_t[KEY_LENGTH])keyCode;

- (void)setGetText:(char* (^)(void))func;

- (bool)getKey:(uint8_t)index;
- (void) clearKey:(uint8_t) index;

- (void)setKeyCodesAtIndex:(int)index to:(uint8_t)keyCode;
- (uint8_t)getKeyCodeAtIndex:(int)index;

- (float) getMouseDeltaX;

- (float) getMouseDeltaY;

- (void) clearDeltas;

- (CGPoint) getMousePos;

- (float*) getScrollingWheelDeltaY;
- (void) setScrollingBottomDistTo:(float)value;
- (void) disableScrolling;

- (void) resetLastKey;

- (UInt16) getLastKey;

- (void) setInTextFeild:(bool) isInFeild;

- (bool) isTextEmpty;

- (void*) getText;
- (void*) getPreformattedEffectMap;
- (void*) getPreformattedEffectMapText;

- (void) clearText;

@end
