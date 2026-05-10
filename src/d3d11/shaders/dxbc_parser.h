#pragma once
#include "core/shaders/parsed_shader.h"
#include "d3d11/shaders/dxbc.h"

namespace d3dsw {

// DXBC container parser (D3D10+ bytecode format). Populates a D3DSW_ParsedShader
// by walking DXBC chunks (SHDR/SHEX for instructions via SM4Decode, ISGN/OSGN
// for signatures, RDEF for resource definitions, STAT/SFI0 for flags).
D3DSW_API Bool DXBCParse(const void* bytecode, Usize len, D3DSW_ParsedShader& out);
D3DSW_API Bool DXBCParseReflection(const void* bytecode, Usize len, D3DSW_ParsedShader& out);

}
