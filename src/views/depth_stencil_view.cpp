#include "views/depth_stencil_view.h"
#include "views/view_util.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11DepthStencilViewSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11DepthStencilView))
    {
        *ppv = static_cast<ID3D11DepthStencilView*>(this);
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

D3D11DepthStencilViewSW::D3D11DepthStencilViewSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

D3D11DepthStencilViewSW::~D3D11DepthStencilViewSW()
{
    if (_resource)
    {
        _resource->Release();
    }
}

HRESULT D3D11DepthStencilViewSW::Init(ID3D11Resource* pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    _resource = pResource;
    _resource->AddRef();

    D3D11SW_RESOURCE_DESC res = GetResourceDesc(pResource);
    _desc    = pDesc ? *pDesc : MakeDefaultDSVDesc(res);

    UINT subresource = CalcDSVSubresource(_desc, res.MipLevels);
    _dataPtr = GetSwDataPtr(pResource, subresource);
    _layout  = GetSwSubresourceLayout(pResource, subresource);
    return S_OK;
}

void STDMETHODCALLTYPE D3D11DepthStencilViewSW::GetResource(ID3D11Resource** ppResource)
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

void STDMETHODCALLTYPE D3D11DepthStencilViewSW::GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = _desc;
    }
}

}
