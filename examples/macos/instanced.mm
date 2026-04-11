#ifdef __APPLE__

#import <AppKit/AppKit.h>
#include "instanced_render.h"

@interface InstancedDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow* window;
@property (strong) NSTimer* timer;
@end

@implementation InstancedDelegate
{
    InstancedApp _app;
    CFAbsoluteTime _startTime;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    NSRect frame = NSMakeRect(200, 200, 800, 600);
    _window = [[NSWindow alloc]
        initWithContentRect:frame
        styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
        backing:NSBackingStoreBuffered
        defer:NO];
    [_window setTitle:@"d3d11sw instanced cubes"];
    [_window makeKeyAndOrderFront:nil];

    memset(&_app, 0, sizeof(_app));
    if (!InstancedInit(_app, (__bridge void*)_window, 800, 600))
    {
        [NSApp terminate:nil];
        return;
    }

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
    InstancedRenderFrame(_app, t);
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    [_timer invalidate];
    _timer = nil;
    InstancedShutdown(_app);
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
        InstancedDelegate* delegate = [[InstancedDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}

#endif
