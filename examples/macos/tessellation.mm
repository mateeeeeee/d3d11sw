#ifdef __APPLE__

#import <AppKit/AppKit.h>
#include "tessellation_render.h"
#include <cmath>

@interface TessView : NSView
@end

@implementation TessView
- (BOOL)acceptsFirstResponder { return YES; }
@end

@interface TessDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow* window;
@property (strong) NSTimer* timer;
@end

@implementation TessDelegate
{
    TessApp _app;
    CFAbsoluteTime _lastTime;
    bool _keys[256];
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    NSRect frame = NSMakeRect(200, 200, 800, 600);
    _window = [[NSWindow alloc]
        initWithContentRect:frame
        styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
        backing:NSBackingStoreBuffered
        defer:NO];
    [_window setTitle:@"d3d11sw tessellation  [WASD orbit, QE zoom]"];

    TessView* view = [[TessView alloc] initWithFrame:frame];
    [_window setContentView:view];
    [_window makeFirstResponder:view];
    [_window makeKeyAndOrderFront:nil];

    memset(&_app, 0, sizeof(_app));
    memset(_keys, 0, sizeof(_keys));
    if (!TessInit(_app, (__bridge void*)_window, 800, 600))
    {
        [NSApp terminate:nil];
        return;
    }

    _lastTime = CFAbsoluteTimeGetCurrent();
    _timer = [NSTimer scheduledTimerWithTimeInterval:1.0 / 60.0
                                              target:self
                                            selector:@selector(tick:)
                                            userInfo:nil
                                             repeats:YES];

    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^NSEvent*(NSEvent* event) {
        if (event.keyCode < 256) self->_keys[event.keyCode] = true;
        // Tab = keyCode 48
        if (event.keyCode == 48) self->_app.wireframe = !self->_app.wireframe;
        return event;
    }];
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp handler:^NSEvent*(NSEvent* event) {
        if (event.keyCode < 256) self->_keys[event.keyCode] = false;
        return event;
    }];
}

- (void)tick:(NSTimer*)timer
{
    CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
    float dt = (float)(now - _lastTime);
    _lastTime = now;

    float rotSpeed = 2.f * dt;
    float zoomSpeed = 8.f * dt;

    // macOS key codes: W=13 S=1 A=0 D=2 Q=12 E=14
    if (_keys[0])  _app.camYaw   -= rotSpeed;  // A
    if (_keys[2])  _app.camYaw   += rotSpeed;  // D
    if (_keys[13]) _app.camPitch += rotSpeed;  // W
    if (_keys[1])  _app.camPitch -= rotSpeed;  // S
    if (_keys[14]) _app.camDist  -= zoomSpeed; // E
    if (_keys[12]) _app.camDist  += zoomSpeed; // Q

    if (_app.camPitch < 0.1f)  _app.camPitch = 0.1f;
    if (_app.camPitch > 1.5f)  _app.camPitch = 1.5f;
    if (_app.camDist  < 2.f)   _app.camDist  = 2.f;
    if (_app.camDist  > 30.f)  _app.camDist  = 30.f;

    TessRenderFrame(_app, dt);

    NSString* mode = _app.wireframe ? @"wireframe" : @"solid";
    [_window setTitle:[NSString stringWithFormat:@"d3d11sw tessellation [%@]  WASD: orbit  QE: zoom  Tab: toggle", mode]];
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    [_timer invalidate];
    _timer = nil;
    TessShutdown(_app);
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
        TessDelegate* delegate = [[TessDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}

#endif
