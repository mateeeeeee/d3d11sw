#pragma once
#include "shaders/dxbc_parser.h"
#include <string>

namespace d3d11sw {

D3D11SW_API std::string EmitShader(const D3D11SW_ParsedShader& shader,
                                   const std::string& runtimeHeaderPath);

}
