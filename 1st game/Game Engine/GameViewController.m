//
//  GameViewController.m
//  Game4
//
//  Created by Luke Hickey on 3/25/24.
//

#import "GameViewController.h"
#import "Renderer.h"

@implementation GameViewController
{
    KeyboardMTKView *_view;
    Renderer *_renderer;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    _view = [[KeyboardMTKView alloc] initWithFrame:self.view.bounds];
    _view.translatesAutoresizingMaskIntoConstraints = NO; // Disable translation of autoresizing mask into constraints
    
    _view.device = MTLCreateSystemDefaultDevice();
    
    if (!_view.device) {
        NSLog(@"Metal is not supported on this device");
        self.view = [[NSView alloc] initWithFrame:self.view.frame];
        return;
    }
    
    _renderer = [[Renderer alloc] initWithMetalKitView:_view];
    
    [_renderer mtkView:_view drawableSizeWillChange:_view.drawableSize];
    
    _view.delegate = _renderer;
    _view.preferredFramesPerSecond = 60;
    
    // Add the view as a subview
    [self.view addSubview:_view];
    
    // Add constraints
    NSDictionary *views = NSDictionaryOfVariableBindings(_view);
    [NSLayoutConstraint activateConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|[_view]|"
                                                                                    options:0
                                                                                    metrics:nil
                                                                                      views:views]];
    [NSLayoutConstraint activateConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|[_view]|"
                                                                                    options:0
                                                                                    metrics:nil
                                                                                      views:views]];
    
    // Make the view the first responder
    [self.view.window makeFirstResponder:_view];
}


@end
