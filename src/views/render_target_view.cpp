#include "views/render_target_view.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE Direct3D11RenderTargetViewSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11RenderTargetView1))
        *ppv = static_cast<ID3D11RenderTargetView1*>(this);
    else if (riid == __uuidof(ID3D11RenderTargetView))
        *ppv = static_cast<ID3D11RenderTargetView*>(this);
    else if (riid == __uuidof(ID3D11View))
        *ppv = static_cast<ID3D11View*>(this);
    else if (riid == __uuidof(ID3D11DeviceChild))
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

Direct3D11RenderTargetViewSW::Direct3D11RenderTargetViewSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11RenderTargetViewSW::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
    {
        *ppResource = nullptr;
    }
}

void STDMETHODCALLTYPE Direct3D11RenderTargetViewSW::GetDesc(D3D11_RENDER_TARGET_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE Direct3D11RenderTargetViewSW::GetDesc1(D3D11_RENDER_TARGET_VIEW_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

}
