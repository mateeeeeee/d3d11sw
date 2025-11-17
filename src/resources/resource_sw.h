#pragma once
#include "common/common.h"

namespace d3d11sw {

struct D3D11SW_SUBRESOURCE_LAYOUT
{
    UINT64 Offset;       // byte offset into the flat buffer
    UINT   RowPitch;     // bytes per block-row
    UINT   DepthPitch;   // bytes per depth slice (RowPitch * NumRows)
    UINT   NumRows;      // block-rows (height / BlockSize for BC, else height)
    UINT   NumSlices;    // depth slices (1 for 1D/2D, depth>>mip for 3D)
    UINT   BlockSize;    // texels per block edge: 4 for BC, 1 otherwise
    UINT   PixelStride;  // bytes per pixel or per 4x4 block
};

struct D3D11SW_COM_UUID("0922aec1-5941-49a2-be44-28e90784e331")
IResourceSW : public IUnknown
{
    virtual void*                     GetDataPtr()                                    = 0;
    virtual UINT64                    GetDataSize()                             const = 0;
    virtual UINT                      GetSubresourceCount()                     const = 0;
    virtual D3D11SW_SUBRESOURCE_LAYOUT GetSubresourceLayout(UINT Subresource)   const = 0;
};

}

D3D11SW_DEFINE_UUID(d3d11sw::IResourceSW,
    0x0922aec1, 0x5941, 0x49a2, 0xbe, 0x44, 0x28, 0xe9, 0x07, 0x84, 0xe3, 0x31)
