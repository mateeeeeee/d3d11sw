#pragma once
#include <vector>
#include "core/shaders/shader_abi.h"
#include "core/shaders/parsed_shader.h"

namespace d3dsw {

struct TessellatorOutput
{
    std::vector<SW_float4> domainPoints;
    std::vector<Uint32>    indices;
};

void Tessellate(Uint32 domain, Uint32 partitioning, Uint32 outputPrimitive,
                const Float* tessFactors, TessellatorOutput& output);

void ExtractTessFactors(const SW_HSOutput& hsOut,
                        const std::vector<D3DSW_ShaderSignatureElement>& pcsg,
                        Uint32 domain, Float outFactors[6]);

}
