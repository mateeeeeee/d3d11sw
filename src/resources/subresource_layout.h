#pragma once
#include "common/common.h"

namespace d3d11sw {

struct D3D11SW_SUBRESOURCE_LAYOUT
{
    Uint64 Offset;       //byte offset into the flat buffer
    Uint   RowPitch;     //bytes per block-row
    Uint   DepthPitch;   //bytes per depth slice (RowPitch * NumRows)
    Uint   NumRows;      //block-rows (height / BlockSize for BC, else height)
    Uint   NumSlices;    //depth slices (1 for 1D/2D, depth>>mip for 3D)
    Uint   BlockSize;    //texels per block edge: 4 for BC, 1 otherwise
    Uint   PixelStride;  //bytes per pixel or per 4x4 block
    Uint   SampleCount;  //1 for non-MS, 2 or 4 for MSAA
};

}
