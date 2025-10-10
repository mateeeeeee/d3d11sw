#include "resources/texture1d.h"

MarsTexture1D::MarsTexture1D(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsTexture1D::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE1D;
}

void STDMETHODCALLTYPE MarsTexture1D::SetEvictionPriority(UINT EvictionPriority) {}

UINT STDMETHODCALLTYPE MarsTexture1D::GetEvictionPriority()
{
    return 0;
}

void STDMETHODCALLTYPE MarsTexture1D::GetDesc(D3D11_TEXTURE1D_DESC* pDesc)
{
    if (pDesc)
        *pDesc = {};
}
