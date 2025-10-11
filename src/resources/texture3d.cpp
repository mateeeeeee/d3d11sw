#include "resources/texture3d.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE Direct3D11Texture3DSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11Texture3D1))
        *ppv = static_cast<ID3D11Texture3D1*>(this);
    else if (riid == __uuidof(ID3D11Texture3D))
        *ppv = static_cast<ID3D11Texture3D*>(this);
    else if (riid == __uuidof(ID3D11Resource))
        *ppv = static_cast<ID3D11Resource*>(this);
    else if (riid == __uuidof(ID3D11DeviceChild))
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

Direct3D11Texture3DSW::Direct3D11Texture3DSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11Texture3DSW::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
    {
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
    }
}

void STDMETHODCALLTYPE Direct3D11Texture3DSW::SetEvictionPriority(UINT EvictionPriority) {}

UINT STDMETHODCALLTYPE Direct3D11Texture3DSW::GetEvictionPriority()
{
    return 0;
}

void STDMETHODCALLTYPE Direct3D11Texture3DSW::GetDesc(D3D11_TEXTURE3D_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE Direct3D11Texture3DSW::GetDesc1(D3D11_TEXTURE3D_DESC1* pDesc)
{
    if (pDesc)
        *pDesc = {};
}

}
