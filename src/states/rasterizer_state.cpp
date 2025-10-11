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

D3D11RasterizerStateSW::D3D11RasterizerStateSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE D3D11RasterizerStateSW::GetDesc(D3D11_RASTERIZER_DESC* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE D3D11RasterizerStateSW::GetDesc1(D3D11_RASTERIZER_DESC1* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE D3D11RasterizerStateSW::GetDesc2(D3D11_RASTERIZER_DESC2* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

}
