#include "states/blend_state.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11BlendStateSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11BlendState1))
    {
        *ppv = static_cast<ID3D11BlendState1*>(this);
    }
    else if (riid == __uuidof(ID3D11BlendState))
    {
        *ppv = static_cast<ID3D11BlendState*>(this);
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

D3D11BlendStateSW::D3D11BlendStateSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

HRESULT D3D11BlendStateSW::Init(const D3D11_BLEND_DESC1* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }

    _desc = *pDesc;
    return S_OK;
}

void STDMETHODCALLTYPE D3D11BlendStateSW::GetDesc(D3D11_BLEND_DESC* pDesc)
{
    if (pDesc)
    {
        pDesc->AlphaToCoverageEnable  = _desc.AlphaToCoverageEnable;
        pDesc->IndependentBlendEnable = _desc.IndependentBlendEnable;
        for (Int i = 0; i < 8; i++)
        {
            const auto& src = _desc.RenderTarget[i];
            auto&       dst = pDesc->RenderTarget[i];
            dst.BlendEnable           = src.BlendEnable;
            dst.SrcBlend              = src.SrcBlend;
            dst.DestBlend             = src.DestBlend;
            dst.BlendOp               = src.BlendOp;
            dst.SrcBlendAlpha         = src.SrcBlendAlpha;
            dst.DestBlendAlpha        = src.DestBlendAlpha;
            dst.BlendOpAlpha          = src.BlendOpAlpha;
            dst.RenderTargetWriteMask = src.RenderTargetWriteMask;
        }
    }
}

void STDMETHODCALLTYPE D3D11BlendStateSW::GetDesc1(D3D11_BLEND_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = _desc;
    }
}

}
