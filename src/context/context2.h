#pragma once
#include "context1.h"

class MarsDeviceContext2 : public MarsDeviceContext1
{
public:
    explicit MarsDeviceContext2(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE UpdateTileMappings(ID3D11Resource* pTiledResource, UINT NumTiledResourceRegions, const D3D11_TILED_RESOURCE_COORDINATE* pTiledResourceRegionStartCoordinates, const D3D11_TILE_REGION_SIZE* pTiledResourceRegionSizes, ID3D11Buffer* pTilePool, UINT NumRanges, const UINT* pRangeFlags, const UINT* pTilePoolStartOffsets, const UINT* pRangeTileCounts, UINT Flags) override;
    HRESULT STDMETHODCALLTYPE CopyTileMappings(ID3D11Resource* pDestTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pDestRegionStartCoordinate, ID3D11Resource* pSourceTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pSourceRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pTileRegionSize, UINT Flags) override;
    void STDMETHODCALLTYPE CopyTiles(ID3D11Resource* pTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pTileRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pTileRegionSize, ID3D11Buffer* pBuffer, UINT64 BufferStartOffsetInBytes, UINT Flags) override;
    void STDMETHODCALLTYPE UpdateTiles(ID3D11Resource* pDestTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pDestTileRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pDestTileRegionSize, const void* pSourceTileData, UINT Flags) override;
    HRESULT STDMETHODCALLTYPE ResizeTilePool(ID3D11Buffer* pTilePool, UINT64 NewSizeInBytes) override;
    void STDMETHODCALLTYPE TiledResourceBarrier(ID3D11DeviceChild* pTiledResourceOrViewAccessBeforeBarrier, ID3D11DeviceChild* pTiledResourceOrViewAccessAfterBarrier) override;
    BOOL STDMETHODCALLTYPE IsAnnotationEnabled() override;
    void STDMETHODCALLTYPE SetMarkerInt(LPCWSTR pLabel, INT Data) override;
    void STDMETHODCALLTYPE BeginEventInt(LPCWSTR pLabel, INT Data) override;
    void STDMETHODCALLTYPE EndEvent() override;
};
