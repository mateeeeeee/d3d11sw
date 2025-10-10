#include "resources/texture2d.h"

namespace d3d11sw {


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
