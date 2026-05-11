#pragma once
#include <memory>
#include <vector>
#include "d3d9/common/d3d9_headers.h"
#include "core/common/common.h"
#include "core/common/unknown_impl.h"


namespace d3dsw {

class D3D9DeviceSW;
class D3D9SurfaceSW;
class ISwapChainPresenter;

class D3D9SwapChainSW final : public IDirect3DSwapChain9, private UnknownBase
{
public:
    D3D9SwapChainSW(D3D9DeviceSW* device, const D3DPRESENT_PARAMETERS& params, HWND focusWindow);
    ~D3D9SwapChainSW() override;

    D3D9SurfaceSW* BackBuffer() { return _backbuffer; }

    ULONG   STDMETHODCALLTYPE AddRef() override  { return AddRefImpl(); }
    ULONG   STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE Present(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion, DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE GetFrontBufferData(IDirect3DSurface9* pDestSurface) override;
    HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) override;
    HRESULT STDMETHODCALLTYPE GetRasterStatus(D3DRASTER_STATUS* pRasterStatus) override;
    HRESULT STDMETHODCALLTYPE GetDisplayMode(D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) override;
    HRESULT STDMETHODCALLTYPE GetPresentParameters(D3DPRESENT_PARAMETERS* pPresentationParameters) override;

private:
    D3D9DeviceSW*         _device     = nullptr;
    D3DPRESENT_PARAMETERS _params     = {};
    D3D9SurfaceSW*        _backbuffer = nullptr;
    std::unique_ptr<ISwapChainPresenter> _presenter;
    std::vector<Uint8>    _presentBuffer;     
};

}
