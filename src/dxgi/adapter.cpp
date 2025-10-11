#include "dxgi/adapter.h"
#include <cstring>

namespace d3d11sw {


DXGIAdapterSW::DXGIAdapterSW() {}
DXGIAdapterSW::~DXGIAdapterSW() {}


HRESULT STDMETHODCALLTYPE DXGIAdapterSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(IDXGIAdapter1))
        *ppv = static_cast<IDXGIAdapter1*>(this);
    else if (riid == __uuidof(IDXGIAdapter))
        *ppv = static_cast<IDXGIAdapter*>(this);
    else if (riid == __uuidof(IDXGIObject))
        *ppv = static_cast<IDXGIObject*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}


HRESULT STDMETHODCALLTYPE DXGIAdapterSW::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIAdapterSW::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIAdapterSW::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIAdapterSW::GetParent(REFIID riid, void** ppParent)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIAdapterSW::EnumOutputs(UINT Output, IDXGIOutput** ppOutput)
{
    return DXGI_ERROR_NOT_FOUND;
}

HRESULT STDMETHODCALLTYPE DXGIAdapterSW::GetDesc(DXGI_ADAPTER_DESC* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }
    memset(pDesc, 0, sizeof(*pDesc));
    wcscpy(pDesc->Description, L"d3d11sw Software Adapter");
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGIAdapterSW::CheckInterfaceSupport(REFGUID InterfaceName, LARGE_INTEGER* pUMDVersion)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE DXGIAdapterSW::GetDesc1(DXGI_ADAPTER_DESC1* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }
    memset(pDesc, 0, sizeof(*pDesc));
    wcscpy(pDesc->Description, L"d3d11sw Software Adapter");
    return S_OK;
}

}
