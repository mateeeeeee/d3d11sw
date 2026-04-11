#ifdef __APPLE__

#import <AppKit/AppKit.h>
#include "floor_render.h"

@interface FloorView : NSView
@end

@interface FloorDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow* window;
@property (strong) NSTimer* timer;
@end

@implementation FloorView
{
    FloorApp* _app;
}

- (void)setApp:(FloorApp*)app { _app = app; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)keyDown:(NSEvent*)event
{
    if (event.keyCode == 49 && _app)
    {
        FloorToggleLod(*_app);
    }
}

@end

@implementation FloorDelegate
{
    FloorApp _app;
    CFAbsoluteTime _startTime;
    FloorView* _view;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    NSRect frame = NSMakeRect(200, 200, 800, 600);
    _window = [[NSWindow alloc]
        initWithContentRect:frame
        styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
        backing:NSBackingStoreBuffered
        defer:NO];
    [_window setTitle:@"d3d11sw floor — Space to toggle LOD"];

    _view = [[FloorView alloc] initWithFrame:frame];
    [_window setContentView:_view];
    [_window makeFirstResponder:_view];
    [_window makeKeyAndOrderFront:nil];

    memset(&_app, 0, sizeof(_app));
    if (!FloorInit(_app, (__bridge void*)_window, 800, 600))
    {
        [NSApp terminate:nil];
        return;
    }
    [_view setApp:&_app];

    _startTime = CFAbsoluteTimeGetCurrent();
    _timer = [NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0
                                              target:self
                                            selector:@selector(tick:)
                                            userInfo:nil
                                             repeats:YES];
}

- (void)tick:(NSTimer*)timer
{
    float t = (float)(CFAbsoluteTimeGetCurrent() - _startTime);
    FloorRenderFrame(_app, t);
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    [_timer invalidate];
    _timer = nil;
    FloorShutdown(_app);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    return YES;
}

@end

int main(int argc, const char* argv[])
{
    @autoreleasepool
    {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        FloorDelegate* delegate = [[FloorDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}

#endif
