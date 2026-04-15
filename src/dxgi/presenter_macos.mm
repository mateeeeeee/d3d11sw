#ifdef D3D11SW_PLATFORM_MACOS

#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>

#include "dxgi/presenter.h"

namespace d3d11sw {

class MacOSPresenter final : public ISwapChainPresenter
{
public:
    explicit MacOSPresenter(NSWindow* window)
        : _window(window)
    {
        NSView* view = [_window contentView];
        if (!view.wantsLayer)
        {
            view.wantsLayer = YES;
        }
    }

    void Present(const void* bgraData, uint32_t width, uint32_t height, uint32_t rowPitch) override
    {
        NSView* view = [_window contentView];
        if (!view || !view.layer)
        {
            return;
        }

        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CFDataRef data = CFDataCreate(nullptr, (const UInt8*)bgraData, (CFIndex)rowPitch * height);
        CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
        CFRelease(data);

        CGImageRef image = CGImageCreate(
            width, height,
            8, 32, rowPitch,
            colorSpace,
            kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst,
            provider,
            nullptr, false,
            kCGRenderingIntentDefault);

        view.layer.contents = (__bridge id)image;

        CGImageRelease(image);
        CGDataProviderRelease(provider);
        CGColorSpaceRelease(colorSpace);
    }

private:
    NSWindow* _window;
};

std::unique_ptr<ISwapChainPresenter> CreatePresenter(void* hwnd)
{
    if (!hwnd)
    {
        return nullptr;
    }
    return std::make_unique<MacOSPresenter>((__bridge NSWindow*)hwnd);
}

void GetWindowClientSize(void* nativeWindow, uint32_t& width, uint32_t& height)
{
    width = 0;
    height = 0;
    if (nativeWindow)
    {
        NSWindow* window = (__bridge NSWindow*)nativeWindow;
        NSRect frame = [[window contentView] frame];
        CGFloat scale = [window backingScaleFactor];
        width  = static_cast<uint32_t>(frame.size.width * scale);
        height = static_cast<uint32_t>(frame.size.height * scale);
    }
}

}

#endif
