#include "dxgi/device.h"
#include "dxgi/adapter.h"

namespace d3d11sw {


DXGIDeviceSW::DXGIDeviceSW(ID3D11Device* device)
    : _device(device)
{
    if (_device)
    {
        _device->AddRef();
    }
}

DXGIDeviceSW::~DXGIDeviceSW()
{
    if (_device)
    {
        _device->Release();
    }
}


HRESULT STDMETHODCALLTYPE DXGIDeviceSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(IDXGIDevice1))
    {
        *ppv = static_cast<IDXGIDevice1*>(this);
    }
    else if (riid == __uuidof(IDXGIDevice))
    {
        *ppv = static_cast<IDXGIDevice*>(this);
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
    if (!pAdapter)
    {
        return E_INVALIDARG;
    }
    DXGIAdapterSW* adapter = new DXGIAdapterSW();
    *pAdapter = adapter;
    return S_OK;
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
    if (pMaxLatency) 
    {
        *pMaxLatency = 1;
    }
    return S_OK;
}

}
