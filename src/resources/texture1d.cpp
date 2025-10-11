#include "resources/texture1d.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11Texture1DSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11Texture1D))
    {
        *ppv = static_cast<ID3D11Texture1D*>(this);
    }
    else if (riid == __uuidof(ID3D11Resource))
    {
        *ppv = static_cast<ID3D11Resource*>(this);
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

D3D11Texture1DSW::D3D11Texture1DSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE D3D11Texture1DSW::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
    {
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE1D;
    }
}

void STDMETHODCALLTYPE D3D11Texture1DSW::SetEvictionPriority(UINT EvictionPriority) {}

UINT STDMETHODCALLTYPE D3D11Texture1DSW::GetEvictionPriority()
{
    return 0;
}

void STDMETHODCALLTYPE D3D11Texture1DSW::GetDesc(D3D11_TEXTURE1D_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

}
