#include "views/unordered_access_view.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE Direct3D11UnorderedAccessViewSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11UnorderedAccessView1))
        *ppv = static_cast<ID3D11UnorderedAccessView1*>(this);
    else if (riid == __uuidof(ID3D11UnorderedAccessView))
        *ppv = static_cast<ID3D11UnorderedAccessView*>(this);
    else if (riid == __uuidof(ID3D11View))
        *ppv = static_cast<ID3D11View*>(this);
    else if (riid == __uuidof(ID3D11DeviceChild))
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

Direct3D11UnorderedAccessViewSW::Direct3D11UnorderedAccessViewSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11UnorderedAccessViewSW::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
    {
        *ppResource = nullptr;
    }
}

void STDMETHODCALLTYPE Direct3D11UnorderedAccessViewSW::GetDesc(D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE Direct3D11UnorderedAccessViewSW::GetDesc1(D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

}
