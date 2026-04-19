#pragma once
#include <vector>
#include <algorithm>
#include "resources/subresource_layout.h"
#include "util/format.h"
#include "util/align.h"

namespace d3d11sw {

// Builds the flat texture buffer and per-subresource layouts.
// Layout: mip-major, slices within each mip:
//   mip0_slice0 | mip0_slice1 | ... | mip1_slice0 | ...
// Subresource index = MipSlice + ArraySlice * MipLevels  (D3D11CalcSubresource)
// For 1D/2D textures: depth=1, arraySize=ArraySize
// For 3D textures:    depth=Depth, arraySize=1
inline void BuildTextureLayouts(
    DXGI_FORMAT                        Format,
    Uint                               Width,
    Uint                               Height,
    Uint                               Depth,
    Uint                               MipLevels,
    Uint                               ArraySize,
    std::vector<D3D11SW_SUBRESOURCE_LAYOUT>& outLayouts,
    std::vector<Uint8>&                    outData,
    Uint                               SampleCount = 1)
{
    Uint blockSize   = GetFormatBlockSize(Format);
    Uint pixelStride = GetFormatStride(Format);

    struct MipInfo
    {
        Uint64 Base;
        Uint   RowPitch;
        Uint   DepthPitch;
        Uint   NumRows;
        Uint   NumSlices;
    };

    std::vector<MipInfo> mipInfo(MipLevels);
    Uint64 offset = 0;
    for (Uint mip = 0; mip < MipLevels; ++mip)
    {
        Uint mipW = std::max(1u, Width  >> mip);
        Uint mipH = std::max(1u, Height >> mip);
        Uint mipD = std::max(1u, Depth  >> mip);

        Uint numBlocksX = std::max(1u, DivideAndRoundUp(mipW, blockSize));
        Uint numBlocksY = std::max(1u, DivideAndRoundUp(mipH, blockSize));

        Uint rowPitch   = numBlocksX * pixelStride * SampleCount;
        Uint depthPitch = rowPitch * numBlocksY;

        mipInfo[mip] = { offset, rowPitch, depthPitch, numBlocksY, mipD };
        offset += (Uint64)depthPitch * mipD * ArraySize;
    }

    outData.assign(offset, 0);
    outLayouts.resize(MipLevels * ArraySize);
    for (Uint mip = 0; mip < MipLevels; ++mip)
    {
        auto& mi = mipInfo[mip];
        for (Uint slice = 0; slice < ArraySize; ++slice)
        {
            Uint   subres      = mip + slice * MipLevels;
            Uint64 sliceOffset = mi.Base + (Uint64)slice * mi.DepthPitch * mi.NumSlices;
            outLayouts[subres] = { sliceOffset, mi.RowPitch, mi.DepthPitch,
                                   mi.NumRows, mi.NumSlices, blockSize, pixelStride, SampleCount };
        }
    }
}

inline void CopyInitialData(
    const D3D11_SUBRESOURCE_DATA*              pInitialData,
    Uint                                       SubresourceCount,
    const std::vector<D3D11SW_SUBRESOURCE_LAYOUT>& Layouts,
    std::vector<Uint8>&                            Data)
{
    if (!pInitialData) 
    {
        return;
    }

    for (Uint subres = 0; subres < SubresourceCount; ++subres)
    {
        if (!pInitialData[subres].pSysMem)
        {
            continue;
        }

        const auto& layout      = Layouts[subres];
        Uint8*      dst         = Data.data() + layout.Offset;
        const Uint8* src        = static_cast<const Uint8*>(pInitialData[subres].pSysMem);
        Uint         srcRowPitch   = pInitialData[subres].SysMemPitch;
        Uint         srcDepthPitch = pInitialData[subres].SysMemSlicePitch;

        for (Uint z = 0; z < layout.NumSlices; ++z)
            for (Uint y = 0; y < layout.NumRows; ++y)
                std::memcpy(
                    dst + (Uint64)z * layout.DepthPitch + (Uint64)y * layout.RowPitch,
                    src + (Uint64)z * srcDepthPitch     + (Uint64)y * srcRowPitch,
                    layout.RowPitch);
    }
}

} 
