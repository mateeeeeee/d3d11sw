#pragma once
#include <vector>
#include "d3d9/common/d3d9_headers.h"

namespace d3dsw {

// Convert an FVF DWORD to an equivalent D3DVERTEXELEMENT9 array (terminated
// by D3DDECL_END). Covers the MVP subset:
//   Position:    XYZ, XYZRHW, XYZW, XYZB1..5 (POSITIONT for XYZRHW)
//   Normal:      NORMAL (FLOAT3)
//   Point size:  PSIZE (FLOAT1)
//   Colors:      DIFFUSE, SPECULAR (D3DCOLOR)
//   Texcoords:   TEX1..TEX8 with per-texcoord FLOAT1/2/3/4 size override via
//                D3DFVF_TEXCOORDSIZEn(i) macros.
void FVFToDeclaration(DWORD fvf, std::vector<D3DVERTEXELEMENT9>& out);

}
