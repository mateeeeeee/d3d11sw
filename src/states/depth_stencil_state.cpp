#include "states/depth_stencil_state.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11DepthStencilStateSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11DepthStencilState))
    {
		*ppv = static_cast<ID3D11DepthStencilState*>(this);
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

D3D11DepthStencilStateSW::D3D11DepthStencilStateSW(ID3D11Device* device) : DeviceChildImpl(device) {}

HRESULT D3D11DepthStencilStateSW::Init(const D3D11_DEPTH_STENCIL_DESC* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }

    _desc = *pDesc;
    return S_OK;
}

void STDMETHODCALLTYPE D3D11DepthStencilStateSW::GetDesc(D3D11_DEPTH_STENCIL_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = _desc;
    }
}

}
