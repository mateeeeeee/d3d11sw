#include "states/rasterizer_state.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11RasterizerStateSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11RasterizerState2))
    {
        *ppv = static_cast<ID3D11RasterizerState2*>(this);
    }
    else if (riid == __uuidof(ID3D11RasterizerState1))
    {
        *ppv = static_cast<ID3D11RasterizerState1*>(this);
    }
    else if (riid == __uuidof(ID3D11RasterizerState))
    {
        *ppv = static_cast<ID3D11RasterizerState*>(this);
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

D3D11RasterizerStateSW::D3D11RasterizerStateSW(ID3D11Device* device) : DeviceChildImpl(device) {}

HRESULT D3D11RasterizerStateSW::Init(const D3D11_RASTERIZER_DESC2* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }

    _desc = *pDesc;
    return S_OK;
}

void STDMETHODCALLTYPE D3D11RasterizerStateSW::GetDesc(D3D11_RASTERIZER_DESC* pDesc)
{
    if (pDesc)
    {
        pDesc->FillMode              = _desc.FillMode;
        pDesc->CullMode              = _desc.CullMode;
        pDesc->FrontCounterClockwise = _desc.FrontCounterClockwise;
        pDesc->DepthBias             = _desc.DepthBias;
        pDesc->DepthBiasClamp        = _desc.DepthBiasClamp;
        pDesc->SlopeScaledDepthBias  = _desc.SlopeScaledDepthBias;
        pDesc->DepthClipEnable       = _desc.DepthClipEnable;
        pDesc->ScissorEnable         = _desc.ScissorEnable;
        pDesc->MultisampleEnable     = _desc.MultisampleEnable;
        pDesc->AntialiasedLineEnable = _desc.AntialiasedLineEnable;
    }
}

void STDMETHODCALLTYPE D3D11RasterizerStateSW::GetDesc1(D3D11_RASTERIZER_DESC1* pDesc)
{
    if (pDesc)
    {
        pDesc->FillMode              = _desc.FillMode;
        pDesc->CullMode              = _desc.CullMode;
        pDesc->FrontCounterClockwise = _desc.FrontCounterClockwise;
        pDesc->DepthBias             = _desc.DepthBias;
        pDesc->DepthBiasClamp        = _desc.DepthBiasClamp;
        pDesc->SlopeScaledDepthBias  = _desc.SlopeScaledDepthBias;
        pDesc->DepthClipEnable       = _desc.DepthClipEnable;
        pDesc->ScissorEnable         = _desc.ScissorEnable;
        pDesc->MultisampleEnable     = _desc.MultisampleEnable;
        pDesc->AntialiasedLineEnable = _desc.AntialiasedLineEnable;
        pDesc->ForcedSampleCount     = _desc.ForcedSampleCount;
    }
}

void STDMETHODCALLTYPE D3D11RasterizerStateSW::GetDesc2(D3D11_RASTERIZER_DESC2* pDesc)
{
    if (pDesc)
    {
        *pDesc = _desc;
    }
}

}
