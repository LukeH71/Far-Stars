

#import "KeyboardMTKView.h"


#include <deque>
#include <iostream>


#define KEY_LENGTH_BYTES int((KEY_LENGTH+7)/8)


@implementation KeyboardMTKView
{
    
    enum ModificationTypes {
        ADD,
        DELETE,
        ARROW,
        SHIFT_ARROW
    };
    
    uint8_t keys[KEY_LENGTH_BYTES];
    uint8_t keysRealTime[KEY_LENGTH_BYTES];
    uint8_t keyCodes[KEY_LENGTH];
    
    CGFloat deltaX;
    CGFloat deltaY;
    
    CGPoint mousePoint;
    
    float scrollingDeltaY;
    float scrollingBottomDist;
    bool scrolling;
    
    UInt16 lastKey;
    uint64_t modifierFlags;
    
    bool inTypingFeild;
    std::vector<KeyStroke> typingFeild;
    std::vector<std::pair<int, std::pair<ModificationTypes, int>>> preFormattedModifications; // index, type, magnitude
    std::vector<int> preFormattedModificationsText;
    
    char* (^getText)(void);
    
}




#pragma mark Initializers, Getters & Setters

-(nonnull instancetype)init;
{
    
    self = [super init];
    if(self)
    {
        
        for(int i = 0; i<KEY_LENGTH_BYTES; i++)
            keys[i]=0;
        
        for(int i = 0; i<KEY_LENGTH_BYTES; i++)
            keysRealTime[i]=0;
        
        
        deltaX = 0;
        deltaY = 0;
        scrollingDeltaY = 0;
        lastKey = 1000;
        scrolling = false;
        
    }
    
    return self;
    
}



-(void)setGetText:(char* (^)())func {
    getText = func;
}

//- (void)setParent:(UIController*)UI {
//    userInterface = UI;
//}


- (void)copy:(id)sender {
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    [pb clearContents];
    
    char *text = getText();
    NSString *selected = [NSString stringWithUTF8String:text];
    [pb setString:selected forType:NSPasteboardTypeString];
    
    delete[] text;
}
//
- (void)cut:(id)sender {
    [self copy:sender];
    preFormattedModifications.push_back({int(typingFeild.size()), {DELETE, -1}});
}
//
- (void)paste:(id)sender {
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    
    const char *text = [[pb stringForType:NSPasteboardTypeString] UTF8String];
    if (text) {
        
        int magnitude = int(strlen(text));
        int prevSize = int(preFormattedModificationsText.size());
        
        preFormattedModifications.push_back({int(typingFeild.size()), {ADD, magnitude}});
        
        preFormattedModificationsText.resize(prevSize+magnitude);
        
        
        for (int i = prevSize; i<prevSize+magnitude; ++i){
            
            switch (text[i-prevSize]) {
                case '0': preFormattedModificationsText[i] = 1; break;
                case '1': preFormattedModificationsText[i] = 2; break;
                case '2': preFormattedModificationsText[i] = 3; break;
                case '3': preFormattedModificationsText[i] = 4; break;
                case '4': preFormattedModificationsText[i] = 5; break;
                case '5': preFormattedModificationsText[i] = 6; break;
                case '6': preFormattedModificationsText[i] = 7; break;
                case '7': preFormattedModificationsText[i] = 8; break;
                case '8': preFormattedModificationsText[i] = 9; break;
                case '9': preFormattedModificationsText[i] = 10; break;
                case 'a': preFormattedModificationsText[i] = 11; break;
                case 'b': preFormattedModificationsText[i] = 12; break;
                case 'c': preFormattedModificationsText[i] = 13; break;
                case 'd': preFormattedModificationsText[i] = 14; break;
                case 'e': preFormattedModificationsText[i] = 15; break;
                case 'f': preFormattedModificationsText[i] = 16; break;
                case 'g': preFormattedModificationsText[i] = 17; break;
                case 'h': preFormattedModificationsText[i] = 18; break;
                case 'i': preFormattedModificationsText[i] = 19; break;
                case 'j': preFormattedModificationsText[i] = 20; break;
                case 'k': preFormattedModificationsText[i] = 21; break;
                case 'l': preFormattedModificationsText[i] = 22; break;
                case 'm': preFormattedModificationsText[i] = 23; break;
                case 'n': preFormattedModificationsText[i] = 24; break;
                case 'o': preFormattedModificationsText[i] = 25; break;
                case 'p': preFormattedModificationsText[i] = 26; break;
                case 'q': preFormattedModificationsText[i] = 27; break;
                case 'r': preFormattedModificationsText[i] = 28; break;
                case 's': preFormattedModificationsText[i] = 29; break;
                case 't': preFormattedModificationsText[i] = 30; break;
                case 'u': preFormattedModificationsText[i] = 31; break;
                case 'v': preFormattedModificationsText[i] = 32; break;
                case 'w': preFormattedModificationsText[i] = 33; break;
                case 'x': preFormattedModificationsText[i] = 34; break;
                case 'y': preFormattedModificationsText[i] = 35; break;
                case 'z': preFormattedModificationsText[i] = 36; break;
                case 'A': preFormattedModificationsText[i] = 37; break;
                case 'B': preFormattedModificationsText[i] = 38; break;
                case 'C': preFormattedModificationsText[i] = 39; break;
                case 'D': preFormattedModificationsText[i] = 40; break;
                case 'E': preFormattedModificationsText[i] = 41; break;
                case 'F': preFormattedModificationsText[i] = 42; break;
                case 'G': preFormattedModificationsText[i] = 43; break;
                case 'H': preFormattedModificationsText[i] = 44; break;
                case 'I': preFormattedModificationsText[i] = 45; break;
                case 'J': preFormattedModificationsText[i] = 46; break;
                case 'K': preFormattedModificationsText[i] = 47; break;
                case 'L': preFormattedModificationsText[i] = 48; break;
                case 'M': preFormattedModificationsText[i] = 49; break;
                case 'N': preFormattedModificationsText[i] = 50; break;
                case 'O': preFormattedModificationsText[i] = 51; break;
                case 'P': preFormattedModificationsText[i] = 52; break;
                case 'Q': preFormattedModificationsText[i] = 53; break;
                case 'R': preFormattedModificationsText[i] = 54; break;
                case 'S': preFormattedModificationsText[i] = 55; break;
                case 'T': preFormattedModificationsText[i] = 56; break;
                case 'U': preFormattedModificationsText[i] = 57; break;
                case 'V': preFormattedModificationsText[i] = 58; break;
                case 'W': preFormattedModificationsText[i] = 59; break;
                case 'X': preFormattedModificationsText[i] = 60; break;
                case 'Y': preFormattedModificationsText[i] = 61; break;
                case 'Z': preFormattedModificationsText[i] = 62; break;
                case '_': preFormattedModificationsText[i] = 64; break;
                case '.': preFormattedModificationsText[i] = 65; break;
                case '`': preFormattedModificationsText[i] = 66; break;
                case '~': preFormattedModificationsText[i] = 67; break;
                case '!': preFormattedModificationsText[i] = 68; break;
                case '@': preFormattedModificationsText[i] = 69; break;
                case '#': preFormattedModificationsText[i] = 70; break;
                case '$': preFormattedModificationsText[i] = 71; break;
                case '%': preFormattedModificationsText[i] = 72; break;
                case '^': preFormattedModificationsText[i] = 73; break;
                case '&': preFormattedModificationsText[i] = 74; break;
                case '*': preFormattedModificationsText[i] = 75; break;
                case '(': preFormattedModificationsText[i] = 76; break;
                case ')': preFormattedModificationsText[i] = 77; break;
                case '-': preFormattedModificationsText[i] = 78; break;
                case '+': preFormattedModificationsText[i] = 80; break;
                case '=': preFormattedModificationsText[i] = 81; break;
                case '{': preFormattedModificationsText[i] = 82; break;
                case '}': preFormattedModificationsText[i] = 83; break;
                case '[': preFormattedModificationsText[i] = 84; break;
                case ']': preFormattedModificationsText[i] = 85; break;
                case '\\': preFormattedModificationsText[i] = 86; break;
                case '|': preFormattedModificationsText[i] = 87; break;
                case ':': preFormattedModificationsText[i] = 88; break;
                case ';': preFormattedModificationsText[i] = 89; break;
                case '\'': preFormattedModificationsText[i] = 90; break;
                case '\"': preFormattedModificationsText[i] = 91; break;
                case ',': preFormattedModificationsText[i] = 92; break;
                case '<': preFormattedModificationsText[i] = 93; break;
                case '>': preFormattedModificationsText[i] = 94; break;
                case '/': preFormattedModificationsText[i] = 95; break;
                case '?': preFormattedModificationsText[i] = 96; break;
                case ' ': preFormattedModificationsText[i] = 98; break;
                    
                default: break;
            }
        }
        //preFormattedEffectMapText
        
        //[self insertTextAtCursor:str]; // your method
    }
}
//
- (void)selectAll:(id)sender {
    
    preFormattedModifications.push_back({int(typingFeild.size()), {ARROW, -INT16_MAX}});
    
    preFormattedModifications.push_back({int(typingFeild.size()), {SHIFT_ARROW, INT16_MAX}});
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
    SEL action = item.action;
    
    if (action == @selector(copy:) || action == @selector(cut:))
        return true;  // only enable when something selected
    
    if (action == @selector(paste:)) {
        NSPasteboard *pb = [NSPasteboard generalPasteboard];
        // Check if pasteboard has anything we can actually consume
        return [pb availableTypeFromArray:@[NSPasteboardTypeString,
                                            NSPasteboardTypeRTF]] != nil;
    }
    
    if (action == @selector(selectAll:))
        return 5 > 0;
    
    return true;
}







// Initialize the keys and keyCodes variables
- (void)initKeys:(uint8_t[KEY_LENGTH])keyCode {
    
    for(int i = 0; i<KEY_LENGTH; i++)
        keyCodes[i]=keyCode[i];
    
}

// Getters and setters below should be self explenitory

- (void)setKeyCodesAtIndex:(int)index to:(uint8_t)keyCode {
    keyCodes[index] = keyCode;
}

- (uint8_t)getKeyCodeAtIndex:(int)index {
    return keyCodes[index];
}

- (float) getMouseDeltaX {
    return deltaX;
}


- (float) getMouseDeltaY {
    return deltaY;
}

- (void) setScrollingBottomDistTo:(float)value {
    scrollingBottomDist = value;
    scrolling = true;
}

- (void) disableScrolling {
    scrolling = false;
    scrollingDeltaY = 0;
}



- (void) clearDeltas {
    deltaX = 0;
    deltaY = 0;
}

// A getter for the mouse position, but inside Metal coodinates for ease of use
- (CGPoint) getMousePos {
    CGPoint metalMousePos = mousePoint;
    NSSize windowFrame = self.window.frame.size;
    
    // Handle the title bar being included inside of the window frame bounds
    if( (self.window.styleMask & NSWindowStyleMaskFullScreen) != NSWindowStyleMaskFullScreen )
        windowFrame.height -= 30;
    
    metalMousePos.x = (metalMousePos.x / windowFrame.width) * 2.0 - 1.0;
    metalMousePos.y = (metalMousePos.y / windowFrame.height) * 2.0 - 1.0;
    
    return metalMousePos;
}

- (float*) getScrollingWheelDeltaY {
    scrolling = true;
    return &scrollingDeltaY;
    
}

- (void) resetLastKey {
    lastKey = 1000;
}

- (UInt16) getLastKey {
    return lastKey;
}

// Get the key using the specified index
- (bool) getKey:(uint8_t) index {
    
    bool temp = keys[index / 8] & (1 << (index % 8));
    
    if(!(keysRealTime[index / 8] & (1 << (index % 8))) && temp) {
        keys[index / 8] &= ~(1 << (index % 8));
    }
    
    return temp;
}

// Clear the key
- (void) clearKey:(uint8_t) index {
    
    keysRealTime[index / 8] &= ~(1 << (index % 8));
    keys[index / 8] &= ~(1 << (index % 8));
      
}


// Handle text feilds
- (void) setInTextFeild:(bool) isInFeild {
    inTypingFeild = isInFeild;
}

- (bool) isTextEmpty {
    return typingFeild.empty() && preFormattedModifications.empty();
}

- (void*) getText {
    if(typingFeild.empty())
        return nullptr;
    
    return &typingFeild;
}

- (void*) getPreformattedEffectMap {
    if(preFormattedModifications.empty())
        return nullptr;
    
    return &preFormattedModifications;
}

- (void*) getPreformattedEffectMapText {
    if(preFormattedModificationsText.empty())
        return nullptr;
    
    return &preFormattedModificationsText;
}

- (void) clearText {
    typingFeild.clear();
    preFormattedModifications.clear();
    preFormattedModificationsText.clear();
}


#pragma mark Handeling Keyboard Input



// Keyboard input for regular keys (ex. K/J)
- (void)keyDown:(NSEvent *)event {
    
    lastKey = event.keyCode;
    
    if(inTypingFeild) // Could add support for holding down keys = several keypresses
        typingFeild.push_back(KeyStroke(lastKey, modifierFlags));
    
    for(uint8_t i = 0; i<KEY_LENGTH; i++) {
        if(keyCodes[i]==event.keyCode) {
            
            keys[i / 8] |= 1 << (i % 8);
            keysRealTime[i / 8] |= 1 << (i % 8);
            break;
        }
    }
}

- (void)keyUp:(NSEvent *)event {
    for(uint8_t i = 0; i<KEY_LENGTH; i++) {
        if(keyCodes[i]==event.keyCode) {
            keysRealTime[i / 8] &= ~(1 << (i % 8));
            break;
        }
    }
}

// Map key codes to their respected modifier flags
NSInteger getModifierFlag(short keyCode) {
    switch (keyCode) {
        case 57:
            return(NSEventModifierFlagCapsLock);
        case 56:
            return(NSEventModifierFlagShift);
        case 60:
            return(NSEventModifierFlagShift);
        case 59:
            return(NSEventModifierFlagControl);
        case 62:
            return(NSEventModifierFlagControl);
        case 58:
            return(NSEventModifierFlagOption);
        case 61:
            return(NSEventModifierFlagOption);
        case 55:
            return(NSEventModifierFlagCommand);
        default:
            return(0);
    }
}

// Keyboard input for modifier keys (ex. Shift/Crtl)
- (void)flagsChanged:(NSEvent *)event {
    
    if (event.keyCode==63) { return; } // disregard the fn key
    
    lastKey = event.keyCode;
    modifierFlags = event.modifierFlags;
    
    for(uint8_t i = 0; i<KEY_LENGTH; i++) {
        if(keyCodes[i]==event.keyCode) {
            if(event.modifierFlags & getModifierFlag(event.keyCode)) {
                keys[i / 8] |= 1 << (i % 8);
                keysRealTime[i / 8] |= 1 << (i % 8);
            } else {
                keysRealTime[i / 8] &= ~(1 << (i % 8));
            }
            break;
        }
    }
    
}




#pragma mark Handeling Mouse Clicks


// Base code for all mouse button down events
- (void)mouseButtonDown:(NSInteger) buttonNumber {
    lastKey = buttonNumber;
    for(uint8_t i = 0; i<KEY_LENGTH; i++) {
        if(keyCodes[i]==buttonNumber) {
            keys[i / 8] |= 1 << (i % 8);
            keysRealTime[i / 8] |= 1 << (i % 8);
            return;
        }
    }
}

// Base code for all mouse button up events
- (void)mouseButtonUp:(NSInteger) buttonNumber {
    for(uint8_t i = 0; i<KEY_LENGTH; i++) {
        if(keyCodes[i]==buttonNumber) {
            keysRealTime[i / 8] &= ~(1 << (i % 8));
            return;
        }
    }
}

// 200: left mouse button
- (void)mouseDown:(NSEvent *)event {
    [self mouseButtonDown: event.buttonNumber+200];
}

// 200: left mouse button
-(void)mouseUp:(NSEvent *)event {
    [self mouseButtonUp: event.buttonNumber+200];
}

// 201: right mouse button
- (void)rightMouseDown:(NSEvent *)event {
    [self mouseButtonDown: event.buttonNumber+200];
}

// 201: right mouse button
-(void)rightMouseUp:(NSEvent *)event {
    [self mouseButtonUp: event.buttonNumber+200];
}

// 202 - 255: 202 = middle, rest is extra buttons
- (void)otherMouseDown:(NSEvent *)event {
    [self mouseButtonDown: event.buttonNumber+200];
}

// 202 - 255: 202 = middle, rest is extra buttons
-(void)otherMouseUp:(NSEvent *)event {
    [self mouseButtonUp: event.buttonNumber+200];
}


#pragma mark Other Mouse Events



// Mouse moving
- (void)mouseMoved:(NSEvent *)event {
    deltaX += event.deltaX;
    deltaY += event.deltaY;
    
    
    mousePoint = event.locationInWindow;
}

- (void)mouseDragged:(NSEvent *)event {
    mousePoint = event.locationInWindow;
}
- (void)rightMouseDragged:(NSEvent *)event {
    mousePoint = event.locationInWindow;
}
- (void)otherMouseDragged:(NSEvent *)event {
    mousePoint = event.locationInWindow;
}


// Scroll Wheel
- (void)scrollWheel:(NSEvent *)event {
    if(scrolling){
        
        float change = float(event.scrollingDeltaY) / 64.0; // 1024 feels good on trackpad
        
        if (scrollingDeltaY < 0 && change > 0) {
            change *= pow(512, scrollingDeltaY);
        }
        if (scrollingDeltaY > scrollingBottomDist && change < 0) {
            change *= pow(512, -scrollingDeltaY + scrollingBottomDist);
        }
        
        scrollingDeltaY -= change;
    }
    
}



// Allow keyboard input
- (BOOL)acceptsFirstResponder {
    return YES;
}

@end








//[NSCursor hide];
//CGAssociateMouseAndMouseCursorPosition(false);

//NSRect windowFrame = self.window.frame;
//NSRect screenFrame = self.window.screen.frame;
//
//CGPoint center = CGPointMake(windowFrame.origin.x + windowFrame.size.width / 2.0,
//                             NSHeight(screenFrame) - windowFrame.origin.y - windowFrame.size.height / 2.0);
//
//if( !NSEqualRects(windowFrame, screenFrame) ) center.y += 15 ;
//
//CGWarpMouseCursorPosition(center);
//
//
////[NSCursor hide];
////CGAssociateMouseAndMouseCursorPosition(false);
