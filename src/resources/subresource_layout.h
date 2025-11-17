#pragma once
#include "common/common.h"

namespace d3d11sw {

struct D3D11SW_SUBRESOURCE_LAYOUT
{
    UINT64 Offset;       //byte offset into the flat buffer
    UINT   RowPitch;     //bytes per block-row
    UINT   DepthPitch;   //bytes per depth slice (RowPitch * NumRows)
    UINT   NumRows;      //block-rows (height / BlockSize for BC, else height)
    UINT   NumSlices;    //depth slices (1 for 1D/2D, depth>>mip for 3D)
    UINT   BlockSize;    //texels per block edge: 4 for BC, 1 otherwise
    UINT   PixelStride;  //bytes per pixel or per 4x4 block
};

}
