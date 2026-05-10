#pragma once
#include <string>
#include "core/shaders/parsed_shader.h"

namespace d3dsw {

D3DSW_API std::string EmitShader(const D3DSW_ParsedShader& shader,
                                   const std::string& runtimeHeaderPath);

}
