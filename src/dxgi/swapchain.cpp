#include "dxgi/swapchain.h"
#include "dxgi/presenter.h"
#include "resources/texture2d.h"

namespace d3d11sw {


DXGISwapChainSW::DXGISwapChainSW(ID3D11Device* device, const DXGI_SWAP_CHAIN_DESC& desc)
    : _device(device), _desc(desc)
{
    if (_device)
    {
        _device->AddRef();
    }
    if ((_desc.BufferDesc.Width == 0 || _desc.BufferDesc.Height == 0) && _desc.OutputWindow)
    {
        uint32_t w = 0, h = 0;
        GetWindowClientSize((void*)_desc.OutputWindow, w, h);
        if (_desc.BufferDesc.Width == 0)
        {
            _desc.BufferDesc.Width = w;
        }
        if (_desc.BufferDesc.Height == 0)
        {
            _desc.BufferDesc.Height = h;
        }
    }
    CreateBackbuffer();
    _presenter = CreatePresenter((void*)_desc.OutputWindow);
}

DXGISwapChainSW::~DXGISwapChainSW()
{
    if (_backbuffer)
    {
        _backbuffer->Release();
    }
    if (_device)
    {
        _device->Release();
    }
}


HRESULT DXGISwapChainSW::CreateBackbuffer()
{
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width       = _desc.BufferDesc.Width;
    texDesc.Height      = _desc.BufferDesc.Height;
    texDesc.MipLevels   = 1;
    texDesc.ArraySize   = 1;
    texDesc.Format      = _desc.BufferDesc.Format;
    texDesc.SampleDesc  = { 1, 0 };
    texDesc.Usage       = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags   = D3D11_BIND_RENDER_TARGET;

    return _device->CreateTexture2D(&texDesc, nullptr, &_backbuffer);
}


HRESULT STDMETHODCALLTYPE DXGISwapChainSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(IDXGISwapChain1))
    {
        *ppv = static_cast<IDXGISwapChain1*>(this);
    }
    else if (riid == __uuidof(IDXGISwapChain))
    {
        *ppv = static_cast<IDXGISwapChain*>(this);
    }
    else if (riid == __uuidof(IDXGIDeviceSubObject))
    {
        *ppv = static_cast<IDXGIDeviceSubObject*>(this);
    }
    else if (riid == __uuidof(IDXGIObject))
    {
        *ppv = static_cast<IDXGIObject*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetParent(REFIID riid, void** ppParent)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetDevice(REFIID riid, void** ppDevice)
{
    return _device->QueryInterface(riid, ppDevice);
}


HRESULT STDMETHODCALLTYPE DXGISwapChainSW::Present(UINT SyncInterval, UINT Flags)
{
    if (!_presenter || !_backbuffer)
    {
        return S_OK;
    }

    D3D11Texture2DSW* tex = static_cast<D3D11Texture2DSW*>(_backbuffer);
    D3D11SW_SUBRESOURCE_LAYOUT layout = tex->GetSubresourceLayout(0);
    const Uint8* src = static_cast<const Uint8*>(tex->GetDataPtr()) + layout.Offset;
    Uint w = _desc.BufferDesc.Width;
    Uint h = _desc.BufferDesc.Height;
    if (_desc.BufferDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM ||
        _desc.BufferDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
    {
        _presenter->Present(src, w, h, layout.RowPitch);
    }
    else
    {
        Usize totalBytes = (Usize)layout.RowPitch * h;
        _presentBuffer.resize(totalBytes);
        for (Uint y = 0; y < h; ++y)
        {
            const Uint8* row = src + (Usize)y * layout.RowPitch;
            Uint8* dst = _presentBuffer.data() + (Usize)y * layout.RowPitch;
            for (Uint x = 0; x < w; ++x)
            {
                dst[x * 4 + 0] = row[x * 4 + 2]; // B
                dst[x * 4 + 1] = row[x * 4 + 1]; // G
                dst[x * 4 + 2] = row[x * 4 + 0]; // R
                dst[x * 4 + 3] = row[x * 4 + 3]; // A
            }
        }
        _presenter->Present(_presentBuffer.data(), w, h, layout.RowPitch);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetBuffer(UINT Buffer, REFIID riid, void** ppSurface)
{
    if (Buffer != 0)
    {
        return DXGI_ERROR_INVALID_CALL;
    }
    if (!_backbuffer)
    {
        return DXGI_ERROR_INVALID_CALL;
    }
    return _backbuffer->QueryInterface(riid, ppSurface);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget)
{
    if (pFullscreen)
    {
        *pFullscreen = FALSE;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }
    *pDesc = _desc;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    if (_backbuffer)
    {
        _backbuffer->Release();
        _backbuffer = nullptr;
    }

    if (Width != 0)
    {
        _desc.BufferDesc.Width = Width;
    }
    if (Height != 0)
    {
        _desc.BufferDesc.Height = Height;
    }
    if (NewFormat != DXGI_FORMAT_UNKNOWN)
    {
        _desc.BufferDesc.Format = NewFormat;
    }
    if (BufferCount != 0)
    {
        _desc.BufferCount = BufferCount;
    }
    _desc.Flags = SwapChainFlags;

    return CreateBackbuffer();
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetContainingOutput(IDXGIOutput** ppOutput)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetLastPresentCount(UINT* pLastPresentCount)
{
    if (pLastPresentCount)
    {
        *pLastPresentCount = 0;
    }
    return S_OK;
}


HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetDesc1(DXGI_SWAP_CHAIN_DESC1* pDesc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetHwnd(HWND* pHwnd)
{
    if (pHwnd)
    {
        *pHwnd = _desc.OutputWindow;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetCoreWindow(REFIID refiid, void** ppUnk)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
    return Present(SyncInterval, PresentFlags);
}

BOOL STDMETHODCALLTYPE DXGISwapChainSW::IsTemporaryMonoSupported()
{
    return FALSE;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetRestrictToOutput(IDXGIOutput** ppRestrictToOutput)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::SetBackgroundColor(const DXGI_RGBA* pColor)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetBackgroundColor(DXGI_RGBA* pColor)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::SetRotation(DXGI_MODE_ROTATION Rotation)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetRotation(DXGI_MODE_ROTATION* pRotation)
{
    return E_NOTIMPL;
}

}
