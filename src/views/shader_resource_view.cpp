#include "views/shader_resource_view.h"
#include "views/view_util.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11ShaderResourceViewSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11ShaderResourceView1))
    {
        *ppv = static_cast<ID3D11ShaderResourceView1*>(this);
    }
    else if (riid == __uuidof(ID3D11ShaderResourceView))
    {
        *ppv = static_cast<ID3D11ShaderResourceView*>(this);
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

D3D11ShaderResourceViewSW::D3D11ShaderResourceViewSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

D3D11ShaderResourceViewSW::~D3D11ShaderResourceViewSW()
{
    if (_resource)
    {
        _resource->Release();
    }
}

HRESULT D3D11ShaderResourceViewSW::Init(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    _resource = pResource;
    _resource->AddRef();

    D3D11SW_RESOURCE_INFO info = GetSWResourceInfo(pResource);
    _desc    = pDesc ? *pDesc : MakeDefaultSRVDesc(info);

    UINT subresource = CalcSRVSubresource(_desc, info.MipLevels);
    _dataPtr = GetSwDataPtr(pResource, subresource);
    _layout  = GetSwSubresourceLayout(pResource, subresource);
    return S_OK;
}

void STDMETHODCALLTYPE D3D11ShaderResourceViewSW::GetResource(ID3D11Resource** ppResource)
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

void STDMETHODCALLTYPE D3D11ShaderResourceViewSW::GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        std::memcpy(pDesc, &_desc, sizeof(*pDesc));
    }
}

void STDMETHODCALLTYPE D3D11ShaderResourceViewSW::GetDesc1(D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = _desc;
    }
}

}
