#include "resources/texture1d.h"

namespace d3d11sw {


Direct3D11Texture1DSW::Direct3D11Texture1DSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11Texture1DSW::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE1D;
}

void STDMETHODCALLTYPE Direct3D11Texture1DSW::SetEvictionPriority(UINT EvictionPriority) {}

UINT STDMETHODCALLTYPE Direct3D11Texture1DSW::GetEvictionPriority()
{
    return 0;
}

void STDMETHODCALLTYPE Direct3D11Texture1DSW::GetDesc(D3D11_TEXTURE1D_DESC* pDesc)
{
    if (pDesc)
        *pDesc = {};
}

}
