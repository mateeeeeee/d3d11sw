#include "device2.h"
#include "context/context.h"

void STDMETHODCALLTYPE MarsDevice2::GetImmediateContext2(
    ID3D11DeviceContext2** ppImmediateContext)
{
    if (ppImmediateContext && m_immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext2*>(m_immediateContext);
        m_immediateContext->AddRef();
    }
}

HRESULT STDMETHODCALLTYPE MarsDevice2::CreateDeferredContext2(
    UINT ContextFlags,
    ID3D11DeviceContext2** ppDeferredContext)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE MarsDevice2::GetResourceTiling(
    ID3D11Resource* pTiledResource,
    UINT* pNumTilesForEntireResource,
    D3D11_PACKED_MIP_DESC* pPackedMipDesc,
    D3D11_TILE_SHAPE* pStandardTileShapeForNonPackedMips,
    UINT* pNumSubresourceTilings,
    UINT FirstSubresourceTilingToGet,
    D3D11_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips)
{
}

HRESULT STDMETHODCALLTYPE MarsDevice2::CheckMultisampleQualityLevels1(
    DXGI_FORMAT Format,
    UINT SampleCount,
    UINT Flags,
    UINT* pNumQualityLevels)
{
    return E_NOTIMPL;
}
