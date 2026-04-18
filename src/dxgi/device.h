#pragma once
#include "dxgi/dxgi_object_impl.h"

namespace d3d11sw {


class DXGIDeviceSW final : public DXGIObjectImpl<IDXGIDevice1>
{
public:
    DXGIDeviceSW(ID3D11Device* device);
    ~DXGIDeviceSW();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent) override;

    HRESULT STDMETHODCALLTYPE GetAdapter(IDXGIAdapter** pAdapter) override;
    HRESULT STDMETHODCALLTYPE CreateSurface(const DXGI_SURFACE_DESC* pDesc, UINT NumSurfaces, DXGI_USAGE Usage, const DXGI_SHARED_RESOURCE* pSharedResource, IDXGISurface** ppSurface) override;
    HRESULT STDMETHODCALLTYPE QueryResourceResidency(IUnknown* const* ppResources, DXGI_RESIDENCY* pResidencyStatus, UINT NumResources) override;
    HRESULT STDMETHODCALLTYPE SetGPUThreadPriority(INT Priority) override;
    HRESULT STDMETHODCALLTYPE GetGPUThreadPriority(INT* pPriority) override;

    HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency) override;
    HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT* pMaxLatency) override;

private:
    ID3D11Device* _device;
    IDXGIAdapter* _adapter = nullptr;
};

}
