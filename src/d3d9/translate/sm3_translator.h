#pragma once
#include "d3d9/translate/sm3_ir.h"
#include "core/shaders/parsed_shader.h"
#include "core/common/common.h"

namespace d3dsw {

// Translate a decoded SM3 program into the SM4-shaped D3DSW_ParsedShader
Bool SM3Translate(const SM3DecodedShader& in, D3DSW_ParsedShader& out);

Bool SM3Parse(const void* bytecode, Usize len, D3DSW_ParsedShader& out);

}
