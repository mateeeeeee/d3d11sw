#pragma once

#include "device1.h"

class MarsDevice2 : public MarsDevice1
{
public:
    void STDMETHODCALLTYPE GetImmediateContext2(
        ID3D11DeviceContext2** ppImmediateContext) override;

    HRESULT STDMETHODCALLTYPE CreateDeferredContext2(
        UINT ContextFlags,
        ID3D11DeviceContext2** ppDeferredContext) override;

    void STDMETHODCALLTYPE GetResourceTiling(
        ID3D11Resource* pTiledResource,
        UINT* pNumTilesForEntireResource,
        D3D11_PACKED_MIP_DESC* pPackedMipDesc,
        D3D11_TILE_SHAPE* pStandardTileShapeForNonPackedMips,
        UINT* pNumSubresourceTilings,
        UINT FirstSubresourceTilingToGet,
        D3D11_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips) override;

    HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels1(
        DXGI_FORMAT Format,
        UINT SampleCount,
        UINT Flags,
        UINT* pNumQualityLevels) override;
};
