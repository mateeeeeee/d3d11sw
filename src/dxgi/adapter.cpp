#include "dxgi/adapter.h"
#include <cstring>

MarsDXGIAdapter::MarsDXGIAdapter() {}
MarsDXGIAdapter::~MarsDXGIAdapter() {}


HRESULT STDMETHODCALLTYPE MarsDXGIAdapter::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIAdapter::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIAdapter::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIAdapter::GetParent(REFIID riid, void** ppParent)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIAdapter::EnumOutputs(UINT Output, IDXGIOutput** ppOutput)
{
    return DXGI_ERROR_NOT_FOUND;
}

HRESULT STDMETHODCALLTYPE MarsDXGIAdapter::GetDesc(DXGI_ADAPTER_DESC* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }
    memset(pDesc, 0, sizeof(*pDesc));
    wcscpy(pDesc->Description, L"MARS Software Adapter");
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MarsDXGIAdapter::CheckInterfaceSupport(REFGUID InterfaceName, LARGE_INTEGER* pUMDVersion)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE MarsDXGIAdapter::GetDesc1(DXGI_ADAPTER_DESC1* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }
    memset(pDesc, 0, sizeof(*pDesc));
    wcscpy(pDesc->Description, L"MARS Software Adapter");
    return S_OK;
}
