#include "shaders/dxbc_parser.h"
#include "shaders/sm4_decoder.h"
#include "common/log.h"
#include <cstring>

namespace d3d11sw {

static Bool ParseSignatureChunk(const Uint8* data, Usize size,
                                std::vector<D3D11SW_ShaderSignatureElement>& out)
{
    if (size < 8)
    {
        return false;
    }

    Uint32 elementCount;
    std::memcpy(&elementCount, data, 4);

    out.reserve(elementCount);
    for (Uint32 i = 0; i < elementCount; ++i)
    {
        const Uint8* rec = data + 8 + i * sizeof(DXBCSigElement);
        if (rec + sizeof(DXBCSigElement) > data + size)
        {
            break;
        }

        DXBCSigElement el{};
        std::memcpy(&el, rec, sizeof(DXBCSigElement));

        D3D11SW_ShaderSignatureElement e{};
        const Char* namePtr = reinterpret_cast<const Char*>(data + el.nameOffset);
        Usize nameLen = 0;
        while (nameLen < 63 && namePtr[nameLen])
        {
            ++nameLen;
        }
        std::memcpy(e.name, namePtr, nameLen);
        e.name[nameLen] = '\0';
        e.semanticIndex = el.semanticIndex;
        e.reg           = el.registerIndex;
        e.mask          = el.mask;
        e.svType        = el.svType;
        out.push_back(e);
    }

    return true;
}

static D3D11SW_ShaderType ShaderTypeFromRdef(Uint16 shaderType)
{
    switch (shaderType)
    {
        case RDEF_SHTYPE_PS: return D3D11SW_ShaderType::Pixel;
        case RDEF_SHTYPE_VS: return D3D11SW_ShaderType::Vertex;
        case RDEF_SHTYPE_GS: return D3D11SW_ShaderType::Geometry;
        case RDEF_SHTYPE_HS: return D3D11SW_ShaderType::Hull;
        case RDEF_SHTYPE_DS: return D3D11SW_ShaderType::Domain;
        case RDEF_SHTYPE_CS: return D3D11SW_ShaderType::Compute;
        default:             return D3D11SW_ShaderType::Unknown;
    }
}

static Bool ParseRdefChunk(const Uint8* data, Usize size, D3D11SW_ParsedShader& out)
{
    if (size < sizeof(DXBCRdefHeader))
    {
        return false;
    }

    DXBCRdefHeader hdr{};
    std::memcpy(&hdr, data, sizeof(DXBCRdefHeader));

    out.type = ShaderTypeFromRdef(hdr.shaderType);
    for (Uint32 i = 0; i < hdr.cbufCount; ++i)
    {
        const Uint8* rec = data + hdr.cbufOffset + i * sizeof(DXBCRdefCbuf);
        if (rec + sizeof(DXBCRdefCbuf) > data + size)
        {
            break;
        }

        DXBCRdefCbuf cb{};
        std::memcpy(&cb, rec, sizeof(DXBCRdefCbuf));

        D3D11SW_CBufBinding b{};
        b.slot      = i;
        b.sizeVec4s = (cb.sizeBytes + 15) / 16;
        out.cbufs.push_back(b);
    }

    for (Uint32 i = 0; i < hdr.bindingCount; ++i)
    {
        const Uint8* rec = data + hdr.bindingOffset + i * sizeof(DXBCRdefBinding);
        if (rec + sizeof(DXBCRdefBinding) > data + size)
        {
            break;
        }

        DXBCRdefBinding bd{};
        std::memcpy(&bd, rec, sizeof(DXBCRdefBinding));
        if (bd.inputType == RDEF_SIT_CBUFFER)
        {
            if (bd.bindPoint < out.cbufs.size())
            {
                out.cbufs[bd.bindPoint].slot = bd.bindPoint;
            }
        }
        else if (bd.inputType == RDEF_SIT_TEXTURE)
        {
            D3D11SW_TexBinding tb{};
            tb.slot      = bd.bindPoint;
            tb.dimension = bd.dimension;
            out.textures.push_back(tb);
        }
    }
    return true;
}

static Bool ParseShaderChunk(const Uint8* data, Usize size, D3D11SW_ParsedShader& out)
{
    if (size < 8)
    {
        return false;
    }

    const Uint32* tokens    = reinterpret_cast<const Uint32*>(data);
    Uint32        numDwords = static_cast<Uint32>(size / 4);

    Uint32 threadGroup[3] = {1, 1, 1};
    if (!SM4Decode(tokens, numDwords, out.instrs, out.numTemps, threadGroup,
                   out.tgsm))
    {
        return false;
    }

    out.threadGroupX  = threadGroup[0];
    out.threadGroupY  = threadGroup[1];
    out.threadGroupZ  = threadGroup[2];
    for (const auto& instr : out.instrs)
    {
        if (instr.op == D3D10_SB_OPCODE_DISCARD)
        {
            out.usesDiscard = true;
        }
        for (const auto& operand : instr.operands)
        {
            if (operand.type == D3D10_SB_OPERAND_TYPE_OUTPUT_DEPTH)
            {
                out.writesSVDepth = true;
            }
            if (operand.type == D3D11_SB_OPERAND_TYPE_UNORDERED_ACCESS_VIEW)
            {
                out.usesUAVs = true;
            }
        }
    }

    return true;
}

Bool DXBCParseReflection(const void* bytecode, Usize len, D3D11SW_ParsedShader& out)
{
    if (!bytecode || len < sizeof(DXBCContainerHeader) + 4)
    {
        return false;
    }

    const Uint8* base = static_cast<const Uint8*>(bytecode);
    DXBCContainerHeader hdr{};
    std::memcpy(&hdr, base, sizeof(DXBCContainerHeader));
    if (hdr.magic != DXBC_MAGIC)
    {
        return false;
    }
    if (hdr.version != 1)
    {
        return false;
    }
    if (hdr.totalSize > len)
    {
        return false;
    }

    const Uint32* offsets = reinterpret_cast<const Uint32*>(base + sizeof(DXBCContainerHeader));
    out = {};
    out.threadGroupX = out.threadGroupY = out.threadGroupZ = 1;
    for (Uint32 c = 0; c < hdr.chunkCount; ++c)
    {
        Uint32 chunkOff = offsets[c];
        if (chunkOff + sizeof(DXBCChunkHeader) > len)
        {
            continue;
        }

        DXBCChunkHeader chdr{};
        std::memcpy(&chdr, base + chunkOff, sizeof(DXBCChunkHeader));
        const Uint8* cdata = base + chunkOff + sizeof(DXBCChunkHeader);
        Usize        csz   = chdr.size;
        if (chdr.fourCC == FOURCC_ISGN)
        {
            ParseSignatureChunk(cdata, csz, out.inputs);
        }
        else if (chdr.fourCC == FOURCC_OSGN || chdr.fourCC == FOURCC_OSG5)
        {
            ParseSignatureChunk(cdata, csz, out.outputs);
        }
        else if (chdr.fourCC == FOURCC_RDEF)
        {
            ParseRdefChunk(cdata, csz, out);
        }
    }

    return true;
}

Bool DXBCParse(const void* bytecode, Usize len, D3D11SW_ParsedShader& out)
{
    if (!DXBCParseReflection(bytecode, len, out))
    {
        return false;
    }

    const Uint8* base = static_cast<const Uint8*>(bytecode);
    DXBCContainerHeader hdr{};
    std::memcpy(&hdr, base, sizeof(DXBCContainerHeader));
    const Uint32* offsets = reinterpret_cast<const Uint32*>(base + sizeof(DXBCContainerHeader));
    for (Uint32 c = 0; c < hdr.chunkCount; ++c)
    {
        Uint32 chunkOff = offsets[c];
        if (chunkOff + sizeof(DXBCChunkHeader) > len)
        {
            continue;
        }

        DXBCChunkHeader chdr{};
        std::memcpy(&chdr, base + chunkOff, sizeof(DXBCChunkHeader));
        if (chdr.fourCC == FOURCC_SHDR || chdr.fourCC == FOURCC_SHEX)
        {
            const Uint8* cdata = base + chunkOff + sizeof(DXBCChunkHeader);
            ParseShaderChunk(cdata, chdr.size, out);
            break;
        }
    }

    for (const auto& e : out.outputs)
    {
        if (e.svType == D3D_NAME_CLIP_DISTANCE)
        {
            Uint32 base = e.semanticIndex * 4;
            for (Uint32 c = 0; c < 4; ++c)
            {
                if (e.mask & (1u << c))
                {
                    Uint32 idx = base + c + 1;
                    if (idx > out.numClipDistances)
                    {
                        out.numClipDistances = idx;
                    }
                }
            }
        }
        if (e.svType == D3D_NAME_CULL_DISTANCE)
        {
            Uint32 base = e.semanticIndex * 4;
            for (Uint32 c = 0; c < 4; ++c)
            {
                if (e.mask & (1u << c))
                {
                    Uint32 idx = base + c + 1;
                    if (idx > out.numCullDistances)
                    {
                        out.numCullDistances = idx;
                    }
                }
            }
        }
    }

    return true;
}

}
