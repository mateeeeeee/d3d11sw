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

void STDMETHODCALLTYPE D3D11BlendStateSW::GetDesc(D3D11_BLEND_DESC* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE D3D11BlendStateSW::GetDesc1(D3D11_BLEND_DESC1* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

}
