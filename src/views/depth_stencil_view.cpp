#include "views/depth_stencil_view.h"

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

void STDMETHODCALLTYPE D3D11DepthStencilViewSW::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
    {
        *ppResource = nullptr;
    }
}

void STDMETHODCALLTYPE D3D11DepthStencilViewSW::GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

}
