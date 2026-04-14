#pragma once
#include "shaders/sm4.h"
#include "shaders/dxbc_parser.h"
#include <vector>

namespace d3d11sw {

D3D11SW_API Bool SM4Decode(const Uint32* tokens, Uint32 numDwords, D3D11SW_ParsedShader& out);

}
