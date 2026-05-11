#include <cstring>
#include "d3d9/swapchain/swap_chain.h"
#include "d3d9/device/device.h"
#include "d3d9/resources/surface.h"
#include "d3d9/translate/d3dformat_bridge.h"
#include "core/common/trace.h"
#include "presenter/presenter.h"

namespace d3dsw {

D3D9SwapChainSW::D3D9SwapChainSW(D3D9DeviceSW* device, const D3DPRESENT_PARAMETERS& params, HWND focusWindow)
    : _device(device), _params(params)
{
    HWND window = _params.hDeviceWindow ? _params.hDeviceWindow : focusWindow;

    if ((_params.BackBufferWidth == 0 || _params.BackBufferHeight == 0) && window)
    {
        uint32_t w = 0, h = 0;
        GetWindowClientSize((void*)window, w, h);
        if (_params.BackBufferWidth == 0)  { _params.BackBufferWidth = w; }
        if (_params.BackBufferHeight == 0) { _params.BackBufferHeight = h; }
    }

    // D3DFMT_UNKNOWN in present params means "use current display format" — for
    // MVP we pick a reasonable default.
    D3DFORMAT d3dFmt = _params.BackBufferFormat;
    if (d3dFmt == D3DFMT_UNKNOWN)
    {
        d3dFmt = D3DFMT_A8R8G8B8;
        _params.BackBufferFormat = d3dFmt;
    }
    DXGI_FORMAT dxgiFmt = D3DFormatToDXGI(d3dFmt);

    _backbuffer = new D3D9SurfaceSW(_device, _params.BackBufferWidth, _params.BackBufferHeight,
                                    d3dFmt, dxgiFmt, D3DRTYPE_SURFACE);

    if (window)
    {
        _presenter = CreatePresenter((void*)window);
    }
}

D3D9SwapChainSW::~D3D9SwapChainSW()
{
    if (_backbuffer)
    {
        _backbuffer->Release();
    }
}

HRESULT STDMETHODCALLTYPE D3D9SwapChainSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }
    if (riid == IID_IUnknown || riid == IID_IDirect3DSwapChain9)
    {
        *ppv = static_cast<IDirect3DSwapChain9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE D3D9SwapChainSW::Present(const RECT*, const RECT*, HWND, const RGNDATA*, DWORD)
{
    D3DSW_TRACE_PRESENT("IDirect3DSwapChain9::Present");
    if (!_presenter || !_backbuffer)
    {
        return S_OK;
    }

    const Uint8* src = _backbuffer->DataPtr();
    UINT w  = _backbuffer->Width();
    UINT h  = _backbuffer->Height();
    UINT rp = _backbuffer->RowPitch();
    DXGI_FORMAT fmt = _backbuffer->DxgiFormat();

    if (fmt == DXGI_FORMAT_B8G8R8A8_UNORM ||
        fmt == DXGI_FORMAT_B8G8R8X8_UNORM ||
        fmt == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
    {
        _presenter->Present(src, w, h, rp);
    }
    else
    {
        // Best-effort swizzle for RGBA → BGRA. Other formats (16-bit/HDR) punted.
        Usize totalBytes = static_cast<Usize>(rp) * h;
        _presentBuffer.resize(totalBytes);
        for (UINT y = 0; y < h; ++y)
        {
            const Uint8* rowSrc = src + static_cast<Usize>(y) * rp;
            Uint8* rowDst = _presentBuffer.data() + static_cast<Usize>(y) * rp;
            for (UINT x = 0; x < w; ++x)
            {
                rowDst[x * 4 + 0] = rowSrc[x * 4 + 2];
                rowDst[x * 4 + 1] = rowSrc[x * 4 + 1];
                rowDst[x * 4 + 2] = rowSrc[x * 4 + 0];
                rowDst[x * 4 + 3] = rowSrc[x * 4 + 3];
            }
        }
        _presenter->Present(_presentBuffer.data(), w, h, rp);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9SwapChainSW::GetFrontBufferData(IDirect3DSurface9*) { return D3DERR_NOTAVAILABLE; }

HRESULT STDMETHODCALLTYPE D3D9SwapChainSW::GetBackBuffer(UINT iBackBuffer, D3DBACKBUFFER_TYPE, IDirect3DSurface9** ppBackBuffer)
{
    if (!ppBackBuffer)
    {
        return D3DERR_INVALIDCALL;
    }
    if (iBackBuffer != 0 || !_backbuffer)
    {
        *ppBackBuffer = nullptr;
        return D3DERR_INVALIDCALL;
    }
    _backbuffer->AddRef();
    *ppBackBuffer = _backbuffer;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9SwapChainSW::GetRasterStatus(D3DRASTER_STATUS*) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9SwapChainSW::GetDisplayMode(D3DDISPLAYMODE*)    { return D3DERR_NOTAVAILABLE; }

HRESULT STDMETHODCALLTYPE D3D9SwapChainSW::GetDevice(IDirect3DDevice9** ppDevice)
{
    if (!ppDevice)
    {
        return D3DERR_INVALIDCALL;
    }
    *ppDevice = static_cast<IDirect3DDevice9*>(_device);
    if (_device)
    {
        _device->AddRef();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9SwapChainSW::GetPresentParameters(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    if (!pPresentationParameters)
    {
        return D3DERR_INVALIDCALL;
    }
    *pPresentationParameters = _params;
    return S_OK;
}

}
