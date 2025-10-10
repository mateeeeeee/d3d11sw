#include "context2.h"

MarsDeviceContext2::MarsDeviceContext2(ID3D11Device* device)
    : MarsDeviceContext1(device)
{
}

HRESULT STDMETHODCALLTYPE MarsDeviceContext2::UpdateTileMappings(ID3D11Resource* pTiledResource, UINT NumTiledResourceRegions, const D3D11_TILED_RESOURCE_COORDINATE* pTiledResourceRegionStartCoordinates, const D3D11_TILE_REGION_SIZE* pTiledResourceRegionSizes, ID3D11Buffer* pTilePool, UINT NumRanges, const UINT* pRangeFlags, const UINT* pTilePoolStartOffsets, const UINT* pRangeTileCounts, UINT Flags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDeviceContext2::CopyTileMappings(ID3D11Resource* pDestTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pDestRegionStartCoordinate, ID3D11Resource* pSourceTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pSourceRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pTileRegionSize, UINT Flags)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE MarsDeviceContext2::CopyTiles(ID3D11Resource* pTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pTileRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pTileRegionSize, ID3D11Buffer* pBuffer, UINT64 BufferStartOffsetInBytes, UINT Flags)
{
}

void STDMETHODCALLTYPE MarsDeviceContext2::UpdateTiles(ID3D11Resource* pDestTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pDestTileRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pDestTileRegionSize, const void* pSourceTileData, UINT Flags)
{
}

HRESULT STDMETHODCALLTYPE MarsDeviceContext2::ResizeTilePool(ID3D11Buffer* pTilePool, UINT64 NewSizeInBytes)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE MarsDeviceContext2::TiledResourceBarrier(ID3D11DeviceChild* pTiledResourceOrViewAccessBeforeBarrier, ID3D11DeviceChild* pTiledResourceOrViewAccessAfterBarrier)
{
}

BOOL STDMETHODCALLTYPE MarsDeviceContext2::IsAnnotationEnabled()
{
    return FALSE;
}

void STDMETHODCALLTYPE MarsDeviceContext2::SetMarkerInt(LPCWSTR pLabel, INT Data)
{
}

void STDMETHODCALLTYPE MarsDeviceContext2::BeginEventInt(LPCWSTR pLabel, INT Data)
{
}

void STDMETHODCALLTYPE MarsDeviceContext2::EndEvent()
{
}
