//
//  Renderer.h
//  Game4
//
//  Created by Luke Hickey on 3/25/24.
//


#import <MetalKit/MetalKit.h>
#import "KeyboardMTKView.h"
#include "AudioEngine.h"
#import <Foundation/Foundation.h>

// Our platform independent renderer class.   Implements the MTKViewDelegate protocol which
//   allows it to accept per-frame update and drawable resize callbacks.
@interface Renderer : NSObject <MTKViewDelegate>

- (nonnull instancetype) initWithMetalKitView:(nonnull MTKView*)mtkView;

@property (nonatomic, readonly) BOOL supportsOrderIndependentTransparency;

@property (nonatomic) BOOL enableOrderIndependentTransparency;

@property (nonatomic) BOOL enableRotation;

/// The main GPU of the device.
@property (nonatomic, readonly, nonnull) id<MTLDevice> device;

/// The view that shows the app's rendered Metal content.
@property (nonatomic, readonly, nonnull) KeyboardMTKView* mtkView;

@end

