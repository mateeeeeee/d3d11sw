#include "dxgi/device.h"

namespace d3d11sw {


DXGIDeviceSW::DXGIDeviceSW(ID3D11Device* device)
    : m_device(device)
{
    if (m_device) m_device->AddRef();
}

DXGIDeviceSW::~DXGIDeviceSW()
{
    if (m_device) m_device->Release();
}


HRESULT STDMETHODCALLTYPE DXGIDeviceSW::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIDeviceSW::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIDeviceSW::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIDeviceSW::GetParent(REFIID riid, void** ppParent)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE DXGIDeviceSW::GetAdapter(IDXGIAdapter** pAdapter)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIDeviceSW::CreateSurface(const DXGI_SURFACE_DESC* pDesc, UINT NumSurfaces, DXGI_USAGE Usage, const DXGI_SHARED_RESOURCE* pSharedResource, IDXGISurface** ppSurface)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIDeviceSW::QueryResourceResidency(IUnknown* const* ppResources, DXGI_RESIDENCY* pResidencyStatus, UINT NumResources)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIDeviceSW::SetGPUThreadPriority(INT Priority)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIDeviceSW::GetGPUThreadPriority(INT* pPriority)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE DXGIDeviceSW::SetMaximumFrameLatency(UINT MaxLatency)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGIDeviceSW::GetMaximumFrameLatency(UINT* pMaxLatency)
{
    if (pMaxLatency) *pMaxLatency = 1;
    return S_OK;
}

}
