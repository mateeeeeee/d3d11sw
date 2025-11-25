#pragma once
#include "shaders/dxbc_parser.h"
#include <string>

namespace d3d11sw {

std::string EmitShader(const D3D11SW_ParsedShader& shader,
                       const std::string& runtimeHeaderPath);

}
