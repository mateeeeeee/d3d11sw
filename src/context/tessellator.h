#pragma once
#include "shaders/shader_abi.h"
#include "shaders/dxbc_parser.h"
#include <vector>

namespace d3d11sw {

struct TessellatorOutput
{
    std::vector<SW_float4> domainPoints;
    std::vector<Uint32>    indices;
};

void Tessellate(Uint32 domain, Uint32 partitioning, Uint32 outputPrimitive,
                const Float* tessFactors, TessellatorOutput& output);

void ExtractTessFactors(const SW_HSOutput& hsOut,
                        const std::vector<D3D11SW_ShaderSignatureElement>& pcsg,
                        Uint32 domain, Float outFactors[6]);

}
