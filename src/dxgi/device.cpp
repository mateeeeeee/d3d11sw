#include "dxgi/device.h"

MarsDXGIDevice::MarsDXGIDevice(ID3D11Device* device)
    : m_device(device)
{
    if (m_device) m_device->AddRef();
}

MarsDXGIDevice::~MarsDXGIDevice()
{
    if (m_device) m_device->Release();
}


HRESULT STDMETHODCALLTYPE MarsDXGIDevice::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIDevice::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIDevice::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIDevice::GetParent(REFIID riid, void** ppParent)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE MarsDXGIDevice::GetAdapter(IDXGIAdapter** pAdapter)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIDevice::CreateSurface(const DXGI_SURFACE_DESC* pDesc, UINT NumSurfaces, DXGI_USAGE Usage, const DXGI_SHARED_RESOURCE* pSharedResource, IDXGISurface** ppSurface)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIDevice::QueryResourceResidency(IUnknown* const* ppResources, DXGI_RESIDENCY* pResidencyStatus, UINT NumResources)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIDevice::SetGPUThreadPriority(INT Priority)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIDevice::GetGPUThreadPriority(INT* pPriority)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE MarsDXGIDevice::SetMaximumFrameLatency(UINT MaxLatency)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MarsDXGIDevice::GetMaximumFrameLatency(UINT* pMaxLatency)
{
    if (pMaxLatency) *pMaxLatency = 1;
    return S_OK;
}
