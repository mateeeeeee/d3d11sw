#include "dxgi/adapter.h"
#include <cstring>

namespace d3d11sw {


DXGIAdapterSW::DXGIAdapterSW() {}
DXGIAdapterSW::~DXGIAdapterSW() {}


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
