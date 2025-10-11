#include "states/sampler_state.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11SamplerStateSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11SamplerState))
    {
        *ppv = static_cast<ID3D11SamplerState*>(this);
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

D3D11SamplerStateSW::D3D11SamplerStateSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE D3D11SamplerStateSW::GetDesc(D3D11_SAMPLER_DESC* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

}
