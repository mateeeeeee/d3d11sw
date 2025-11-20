#include "views/unordered_access_view.h"
#include "views/view_util.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11UnorderedAccessViewSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11UnorderedAccessView1))
    {
        *ppv = static_cast<ID3D11UnorderedAccessView1*>(this);
    }
    else if (riid == __uuidof(ID3D11UnorderedAccessView))
    {
        *ppv = static_cast<ID3D11UnorderedAccessView*>(this);
    }
    else if (riid == __uuidof(ID3D11View))
    {
        *ppv = static_cast<ID3D11View*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceChild))
    {
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

D3D11UnorderedAccessViewSW::D3D11UnorderedAccessViewSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

D3D11UnorderedAccessViewSW::~D3D11UnorderedAccessViewSW()
{
    if (_resource)
    {
        _resource->Release();
    }
}

HRESULT D3D11UnorderedAccessViewSW::Init(ID3D11Resource* pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    _resource = pResource;
    _resource->AddRef();

    D3D11SW_RESOURCE_INFO info = GetSWResourceInfo(pResource);
    _desc    = pDesc ? *pDesc : MakeDefaultUAVDesc(info);

    UINT subresource = CalcUAVSubresource(_desc, info.MipLevels);
    _dataPtr = GetSwDataPtr(pResource, subresource);
    _layout  = GetSwSubresourceLayout(pResource, subresource);
    return S_OK;
}

void STDMETHODCALLTYPE D3D11UnorderedAccessViewSW::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
    {
        *ppResource = _resource;
        if (_resource)
        {
            _resource->AddRef();
        }
    }
}

void STDMETHODCALLTYPE D3D11UnorderedAccessViewSW::GetDesc(D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        std::memcpy(pDesc, &_desc, sizeof(*pDesc));
    }
}

void STDMETHODCALLTYPE D3D11UnorderedAccessViewSW::GetDesc1(D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = _desc;
    }
}

}
