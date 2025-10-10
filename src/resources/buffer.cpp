#include "resources/buffer.h"

namespace d3d11sw {


Direct3D11BufferSW::Direct3D11BufferSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11BufferSW::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
    {
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_BUFFER;
    }
}

void STDMETHODCALLTYPE Direct3D11BufferSW::SetEvictionPriority(UINT EvictionPriority) {}

UINT STDMETHODCALLTYPE Direct3D11BufferSW::GetEvictionPriority()
{
    return 0;
}

void STDMETHODCALLTYPE Direct3D11BufferSW::GetDesc(D3D11_BUFFER_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

}
