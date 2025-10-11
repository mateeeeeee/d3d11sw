#include "resources/texture2d.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE Direct3D11Texture2DSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11Texture2D1))
        *ppv = static_cast<ID3D11Texture2D1*>(this);
    else if (riid == __uuidof(ID3D11Texture2D))
        *ppv = static_cast<ID3D11Texture2D*>(this);
    else if (riid == __uuidof(ID3D11Resource))
        *ppv = static_cast<ID3D11Resource*>(this);
    else if (riid == __uuidof(ID3D11DeviceChild))
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

Direct3D11Texture2DSW::Direct3D11Texture2DSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11Texture2DSW::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
    {
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D;
    }
}

void STDMETHODCALLTYPE Direct3D11Texture2DSW::SetEvictionPriority(UINT EvictionPriority) {}

UINT STDMETHODCALLTYPE Direct3D11Texture2DSW::GetEvictionPriority()
{
    return 0;
}

void STDMETHODCALLTYPE Direct3D11Texture2DSW::GetDesc(D3D11_TEXTURE2D_DESC* pDesc)
{
    if (pDesc)
        *pDesc = {};
}

void STDMETHODCALLTYPE Direct3D11Texture2DSW::GetDesc1(D3D11_TEXTURE2D_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

}
