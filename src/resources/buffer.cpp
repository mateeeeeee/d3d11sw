#include "resources/buffer.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11BufferSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11Buffer))
    {
        *ppv = static_cast<ID3D11Buffer*>(this);
    }
    else if (riid == __uuidof(ID3D11Resource))
    {
        *ppv = static_cast<ID3D11Resource*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceChild))
    {
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


D3D11BufferSW::D3D11BufferSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

HRESULT D3D11BufferSW::Init(
    const D3D11_BUFFER_DESC*        pDesc,
    const D3D11_SUBRESOURCE_DATA*   pInitialData)
{
    _desc = *pDesc;
    _data.resize(pDesc->ByteWidth, 0); 
    if (pInitialData && pInitialData->pSysMem)
    {
        std::memcpy(_data.data(), pInitialData->pSysMem, pDesc->ByteWidth);
    }

    return S_OK;
}


void STDMETHODCALLTYPE D3D11BufferSW::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
    {
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_BUFFER;
    }
}

void STDMETHODCALLTYPE D3D11BufferSW::SetEvictionPriority(UINT EvictionPriority) {}

UINT STDMETHODCALLTYPE D3D11BufferSW::GetEvictionPriority()
{
    return 0;
}

void STDMETHODCALLTYPE D3D11BufferSW::GetDesc(D3D11_BUFFER_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = _desc;
    }
}

D3D11SW_SUBRESOURCE_LAYOUT D3D11BufferSW::GetSubresourceLayout(UINT Subresource) const
{
    return { 0, _desc.ByteWidth, _desc.ByteWidth, 1, 1, 1, 1, 1 };
}

}
