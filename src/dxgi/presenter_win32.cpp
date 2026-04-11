#include "common/common.h"
#include "common/log.h"
#include "dxgi/presenter.h"

#ifdef D3D11SW_PLATFORM_WINDOWS

namespace d3d11sw {

class Win32Presenter final : public ISwapChainPresenter
{
public:
    explicit Win32Presenter(HWND hwnd)
        : _hwnd(hwnd)
    {
    }

    void Present(const void* bgraData, uint32_t width, uint32_t height, uint32_t rowPitch) override
    {
        HDC hdc = GetDC(_hwnd);
        if (!hdc)
        {
            D3D11SW_ERROR("Win32Presenter::Present: GetDC failed");
            return;
        }

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = (LONG)width;
        bmi.bmiHeader.biHeight      = -(LONG)height;
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        StretchDIBits(
            hdc,
            0, 0, (int)width, (int)height,
            0, 0, (int)width, (int)height,
            bgraData, &bmi,
            DIB_RGB_COLORS, SRCCOPY);

        ReleaseDC(_hwnd, hdc);
    }

private:
    HWND _hwnd;
};

std::unique_ptr<ISwapChainPresenter> CreatePresenter(void* hwnd)
{
    if (!hwnd)
    {
        D3D11SW_ERROR("CreatePresenter: hwnd is null");
        return nullptr;
    }
    return std::make_unique<Win32Presenter>((HWND)hwnd);
}

}

#endif
