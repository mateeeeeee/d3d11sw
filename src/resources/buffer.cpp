#include "resources/buffer.h"

namespace d3d11sw {


Direct3D11BufferSW::Direct3D11BufferSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

HRESULT Direct3D11BufferSW::Init(
    const D3D11_BUFFER_DESC*        pDesc,
    const D3D11_SUBRESOURCE_DATA*   pInitialData)
{
    if (!pDesc || pDesc->ByteWidth == 0)
    {
        return E_INVALIDARG;
    }

    _desc = *pDesc;
    _data.resize(pDesc->ByteWidth, 0); 

    if (pInitialData && pInitialData->pSysMem)
    {
        std::memcpy(_data.data(), pInitialData->pSysMem, pDesc->ByteWidth);
    }

    return S_OK;
}


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
