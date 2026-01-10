#pragma once
#include "shaders/dxbc_parser.h"
#include <cstring>
#include <vector>

namespace d3d11sw {

inline Int FindSVPositionInput(const D3D11SW_ParsedShader& shader)
{
    for (const auto& e : shader.inputs)
    {
        if (std::strcmp(e.name, "SV_Position") == 0 || std::strcmp(e.name, "SV_POSITION") == 0)
        {
            return static_cast<Int>(e.reg);
        }
    }
    return -1;
}

inline Int FindSemanticRegister(const std::vector<D3D11SW_ShaderSignatureElement>& sig,
                                const Char* name, Uint32 semIdx)
{
    for (const auto& e : sig)
    {
        if (e.semanticIndex == semIdx && std::strcmp(e.name, name) == 0)
        {
            return static_cast<Int>(e.reg);
        }
    }
    return -1;
}

}
