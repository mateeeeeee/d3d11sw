#pragma once
#include "core/shaders/sm4.h"
#include "core/shaders/parsed_shader.h"
#include <vector>

namespace d3dsw {

D3DSW_API Bool SM4Decode(const Uint32* tokens, Uint32 numDwords, D3DSW_ParsedShader& out);

}
