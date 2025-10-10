#include "resources/texture3d.h"

MarsTexture3D::MarsTexture3D(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsTexture3D::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
}

void STDMETHODCALLTYPE MarsTexture3D::SetEvictionPriority(UINT EvictionPriority) {}

UINT STDMETHODCALLTYPE MarsTexture3D::GetEvictionPriority()
{
    return 0;
}

void STDMETHODCALLTYPE MarsTexture3D::GetDesc(D3D11_TEXTURE3D_DESC* pDesc)
{
    if (pDesc)
        *pDesc = {};
}

void STDMETHODCALLTYPE MarsTexture3D::GetDesc1(D3D11_TEXTURE3D_DESC1* pDesc)
{
    if (pDesc)
        *pDesc = {};
}
