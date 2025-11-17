#pragma once
#include <vector>
#include <algorithm>
#include "resources/resource_sw.h"
#include "util/format.h"
#include "util/align.h"

namespace d3d11sw {

// Builds the flat texture buffer and per-subresource layouts.
//
// Layout: mip-major, slices within each mip:
//   mip0_slice0 | mip0_slice1 | ... | mip1_slice0 | ...
//
// Subresource index = MipSlice + ArraySlice * MipLevels  (D3D11CalcSubresource)
//
// For 1D/2D textures: depth=1, arraySize=ArraySize
// For 3D textures:    depth=Depth, arraySize=1
inline void BuildTextureLayouts(
    DXGI_FORMAT                        Format,
    UINT                               Width,
    UINT                               Height,
    UINT                               Depth,
    UINT                               MipLevels,
    UINT                               ArraySize,
    std::vector<D3D11SW_SUBRESOURCE_LAYOUT>& outLayouts,
    std::vector<Uint8>&                    outData)
{
    UINT blockSize   = GetFormatBlockSize(Format);
    UINT pixelStride = GetFormatStride(Format);

    struct MipInfo
    {
        UINT64 Base;
        UINT   RowPitch;
        UINT   DepthPitch;
        UINT   NumRows;
        UINT   NumSlices;
    };

    std::vector<MipInfo> mipInfo(MipLevels);
    UINT64 offset = 0;

    for (UINT mip = 0; mip < MipLevels; ++mip)
    {
        UINT mipW = std::max(1u, Width  >> mip);
        UINT mipH = std::max(1u, Height >> mip);
        UINT mipD = std::max(1u, Depth  >> mip);

        UINT numBlocksX = std::max(1u, DivideAndRoundUp(mipW, blockSize));
        UINT numBlocksY = std::max(1u, DivideAndRoundUp(mipH, blockSize));

        UINT rowPitch   = numBlocksX * pixelStride;
        UINT depthPitch = rowPitch * numBlocksY;

        mipInfo[mip] = { offset, rowPitch, depthPitch, numBlocksY, mipD };
        offset += (UINT64)depthPitch * mipD * ArraySize;
    }

    outData.assign(offset, 0);

    outLayouts.resize(MipLevels * ArraySize);
    for (UINT mip = 0; mip < MipLevels; ++mip)
    {
        auto& mi = mipInfo[mip];
        for (UINT slice = 0; slice < ArraySize; ++slice)
        {
            UINT   subres      = mip + slice * MipLevels;
            UINT64 sliceOffset = mi.Base + (UINT64)slice * mi.DepthPitch * mi.NumSlices;
            outLayouts[subres] = { sliceOffset, mi.RowPitch, mi.DepthPitch,
                                   mi.NumRows, mi.NumSlices, blockSize, pixelStride };
        }
    }
}

// Copies caller-provided initial data into the flat texture buffer.
inline void CopyInitialData(
    const D3D11_SUBRESOURCE_DATA*              pInitialData,
    UINT                                       SubresourceCount,
    const std::vector<D3D11SW_SUBRESOURCE_LAYOUT>& Layouts,
    std::vector<Uint8>&                            Data)
{
    if (!pInitialData) return;

    for (UINT subres = 0; subres < SubresourceCount; ++subres)
    {
        if (!pInitialData[subres].pSysMem) continue;

        const auto& layout      = Layouts[subres];
        Uint8*      dst         = Data.data() + layout.Offset;
        const Uint8* src        = static_cast<const Uint8*>(pInitialData[subres].pSysMem);
        UINT         srcRowPitch   = pInitialData[subres].SysMemPitch;
        UINT         srcDepthPitch = pInitialData[subres].SysMemSlicePitch;

        for (UINT z = 0; z < layout.NumSlices; ++z)
            for (UINT y = 0; y < layout.NumRows; ++y)
                std::memcpy(
                    dst + (UINT64)z * layout.DepthPitch + (UINT64)y * layout.RowPitch,
                    src + (UINT64)z * srcDepthPitch     + (UINT64)y * srcRowPitch,
                    layout.RowPitch);
    }
}

} // namespace d3d11sw
