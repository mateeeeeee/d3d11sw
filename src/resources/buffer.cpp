#include "resources/buffer.h"

MarsBuffer::MarsBuffer(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsBuffer::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_BUFFER;
}

void STDMETHODCALLTYPE MarsBuffer::SetEvictionPriority(UINT EvictionPriority) {}

UINT STDMETHODCALLTYPE MarsBuffer::GetEvictionPriority()
{
    return 0;
}

void STDMETHODCALLTYPE MarsBuffer::GetDesc(D3D11_BUFFER_DESC* pDesc)
{
    if (pDesc)
        *pDesc = {};
}
