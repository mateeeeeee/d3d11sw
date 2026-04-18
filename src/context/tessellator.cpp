#include "context/tessellator.h"
#include <d3dcommon.h>
#include <d3d11TokenizedProgramFormat.hpp>
#include <tessellator.hpp>

namespace d3d11sw {

static Int LowestSetBit(Uint8 mask)
{
    for (Int i = 0; i < 4; ++i)
    {
        if (mask & (1 << i))
        {
            return i;
        }
    }
    return 0;
}

static microsoft::D3D11_TESSELLATOR_PARTITIONING MapPartitioning(Uint32 p)
{
    switch (p)
    {
    case D3D11_SB_TESSELLATOR_PARTITIONING_INTEGER:         return microsoft::D3D11_TESSELLATOR_PARTITIONING_INTEGER;
    case D3D11_SB_TESSELLATOR_PARTITIONING_POW2:            return microsoft::D3D11_TESSELLATOR_PARTITIONING_POW2;
    case D3D11_SB_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD:  return microsoft::D3D11_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD;
    case D3D11_SB_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN: return microsoft::D3D11_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN;
    default:                                                return microsoft::D3D11_TESSELLATOR_PARTITIONING_INTEGER;
    }
}

static microsoft::D3D11_TESSELLATOR_OUTPUT_PRIMITIVE MapOutputPrimitive(Uint32 p)
{
    switch (p)
    {
    case D3D11_SB_TESSELLATOR_OUTPUT_POINT:        return microsoft::D3D11_TESSELLATOR_OUTPUT_POINT;
    case D3D11_SB_TESSELLATOR_OUTPUT_LINE:         return microsoft::D3D11_TESSELLATOR_OUTPUT_LINE;
    case D3D11_SB_TESSELLATOR_OUTPUT_TRIANGLE_CW:  return microsoft::D3D11_TESSELLATOR_OUTPUT_TRIANGLE_CW;
    case D3D11_SB_TESSELLATOR_OUTPUT_TRIANGLE_CCW: return microsoft::D3D11_TESSELLATOR_OUTPUT_TRIANGLE_CCW;
    default:                                       return microsoft::D3D11_TESSELLATOR_OUTPUT_TRIANGLE_CW;
    }
}

void Tessellate(Uint32 domain, Uint32 partitioning, Uint32 outputPrimitive,
                const Float* tessFactors, TessellatorOutput& output)
{
    output.domainPoints.clear();
    output.indices.clear();

    microsoft::CHWTessellator tess;
    tess.Init(MapPartitioning(partitioning), MapOutputPrimitive(outputPrimitive));
    switch (domain)
    {
    case D3D11_SB_TESSELLATOR_DOMAIN_TRI:
        tess.TessellateTriDomain(
            tessFactors[0], tessFactors[1], tessFactors[2],
            tessFactors[3]);
        break;
    case D3D11_SB_TESSELLATOR_DOMAIN_QUAD:
        tess.TessellateQuadDomain(
            tessFactors[0], tessFactors[1], tessFactors[2], tessFactors[3],
            tessFactors[4], tessFactors[5]);
        break;
    case D3D11_SB_TESSELLATOR_DOMAIN_ISOLINE:
        tess.TessellateIsoLineDomain(
            tessFactors[0], tessFactors[1]);
        break;
    default:
        return;
    }

    Int ptCount = tess.GetPointCount();
    Int idxCount = tess.GetIndexCount();
    microsoft::DOMAIN_POINT* pts = tess.GetPoints();
    Int* idx = tess.GetIndices();
    output.domainPoints.resize(ptCount);
    for (Int i = 0; i < ptCount; ++i)
    {
        Float u = pts[i].u;
        Float v = pts[i].v;
        Float w = (domain == D3D11_SB_TESSELLATOR_DOMAIN_TRI) ? (1.f - u - v) : 0.f;
        output.domainPoints[i] = {u, v, w, 0.f};
    }

    output.indices.resize(idxCount);
    for (Int i = 0; i < idxCount; ++i)
    {
        output.indices[i] = static_cast<Uint32>(idx[i]);
    }
}

void ExtractTessFactors(const SW_HSOutput& hsOut,
                        const std::vector<D3D11SW_ShaderSignatureElement>& pcsg,
                        Uint32 domain, Float outFactors[6])
{
    for (Int i = 0; i < 6; ++i)
    {
        outFactors[i] = 0.f;
    }

    for (const auto& e : pcsg)
    {
        const SW_float4& pc = hsOut.patchConstants[e.reg];
        const Float* comps = &pc.x;
        Int comp = LowestSetBit(e.mask);
        switch (e.svType)
        {
        case D3D_NAME_FINAL_QUAD_EDGE_TESSFACTOR:
        case D3D_NAME_FINAL_TRI_EDGE_TESSFACTOR:
        case D3D_NAME_FINAL_LINE_DETAIL_TESSFACTOR:
        case D3D_NAME_FINAL_LINE_DENSITY_TESSFACTOR:
            outFactors[e.semanticIndex] = comps[comp];
            break;
        case D3D_NAME_FINAL_QUAD_INSIDE_TESSFACTOR:
        case D3D_NAME_FINAL_TRI_INSIDE_TESSFACTOR:
        {
            Uint32 base = (e.svType == D3D_NAME_FINAL_QUAD_INSIDE_TESSFACTOR) ? 4 : 3;
            outFactors[base + e.semanticIndex] = comps[comp];
            break;
        }
        }
    }
}

}
