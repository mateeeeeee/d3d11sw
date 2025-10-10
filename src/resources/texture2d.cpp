#include "resources/texture2d.h"

MarsTexture2D::MarsTexture2D(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsTexture2D::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D;
}

void STDMETHODCALLTYPE MarsTexture2D::SetEvictionPriority(UINT EvictionPriority) {}

UINT STDMETHODCALLTYPE MarsTexture2D::GetEvictionPriority()
{
    return 0;
}

void STDMETHODCALLTYPE MarsTexture2D::GetDesc(D3D11_TEXTURE2D_DESC* pDesc)
{
    if (pDesc)
        *pDesc = {};
}

void STDMETHODCALLTYPE MarsTexture2D::GetDesc1(D3D11_TEXTURE2D_DESC1* pDesc)
{
    if (pDesc)
        *pDesc = {};
}
