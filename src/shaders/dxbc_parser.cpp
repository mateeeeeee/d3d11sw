#include "shaders/dxbc_parser.h"
#include "shaders/sm4_decoder.h"
#include "common/log.h"
#include <cstring>

namespace d3d11sw {

static Bool ParseSignatureChunk(const Uint8* data, Usize size,
                                std::vector<D3D11SW_ShaderSignatureElement>& out,
                                Bool osg5 = false)
{
    if (size < 8)
    {
        D3D11SW_ERROR("ParseSignatureChunk: chunk too small ({} < 8)", size);
        return false;
    }

    Uint32 elementCount;
    std::memcpy(&elementCount, data, 4);

    Usize elemSize = osg5 ? sizeof(DXBCSigElement5) : sizeof(DXBCSigElement);

    out.reserve(elementCount);
    for (Uint32 i = 0; i < elementCount; ++i)
    {
        const Uint8* rec = data + 8 + i * elemSize;
        if (rec + elemSize > data + size)
        {
            break;
        }

        D3D11SW_ShaderSignatureElement e{};
        Uint32 nameOffset, semanticIndex, svType, registerIndex;
        Uint8 mask;

        if (osg5)
        {
            DXBCSigElement5 el{};
            std::memcpy(&el, rec, sizeof(DXBCSigElement5));
            nameOffset    = el.nameOffset;
            semanticIndex = el.semanticIndex;
            svType        = el.svType;
            registerIndex = el.registerIndex;
            mask          = el.mask;
        }
        else
        {
            DXBCSigElement el{};
            std::memcpy(&el, rec, sizeof(DXBCSigElement));
            nameOffset    = el.nameOffset;
            semanticIndex = el.semanticIndex;
            svType        = el.svType;
            registerIndex = el.registerIndex;
            mask          = el.mask;
        }

        const Char* namePtr = reinterpret_cast<const Char*>(data + nameOffset);
        Usize nameLen = 0;
        while (nameLen < 63 && namePtr[nameLen])
        {
            ++nameLen;
        }
        std::memcpy(e.name, namePtr, nameLen);
        e.name[nameLen] = '\0';
        e.semanticIndex = semanticIndex;
        e.reg           = registerIndex;
        e.mask          = mask;
        e.svType        = svType;
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
        D3D11SW_ERROR("ParseRdefChunk: chunk too small ({} < {})", size, sizeof(DXBCRdefHeader));
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
        D3D11SW_ERROR("ParseShaderChunk: chunk too small ({} < 8)", size);
        return false;
    }

    const Uint32* tokens    = reinterpret_cast<const Uint32*>(data);
    Uint32        numDwords = static_cast<Uint32>(size / 4);

    if (!SM4Decode(tokens, numDwords, out))
    {
        D3D11SW_ERROR("ParseShaderChunk: SM4Decode failed");
        return false;
    }

    for (const auto& instr : out.instrs)
    {
        if (instr.op == D3D10_SB_OPCODE_DISCARD)
        {
            out.usesDiscard = true;
        }
        if (out.type == D3D11SW_ShaderType::Pixel &&
            (instr.op == D3D10_SB_OPCODE_SAMPLE ||
             instr.op == D3D10_SB_OPCODE_SAMPLE_B ||
             instr.op == D3D10_SB_OPCODE_SAMPLE_C ||
             instr.op == D3D10_SB_OPCODE_DERIV_RTX ||
             instr.op == D3D10_SB_OPCODE_DERIV_RTY ||
             instr.op == D3D11_SB_OPCODE_DERIV_RTX_COARSE ||
             instr.op == D3D11_SB_OPCODE_DERIV_RTY_COARSE ||
             instr.op == D3D11_SB_OPCODE_DERIV_RTX_FINE ||
             instr.op == D3D11_SB_OPCODE_DERIV_RTY_FINE ||
             instr.op == D3D10_1_SB_OPCODE_LOD))
        {
            out.needsQuad = true;
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
        D3D11SW_ERROR("DXBCParseReflection: invalid bytecode (null={}, len={})", !bytecode, len);
        return false;
    }

    const Uint8* base = static_cast<const Uint8*>(bytecode);
    DXBCContainerHeader hdr{};
    std::memcpy(&hdr, base, sizeof(DXBCContainerHeader));
    if (hdr.magic != DXBC_MAGIC)
    {
        D3D11SW_ERROR("DXBCParseReflection: bad magic 0x{:08X}", hdr.magic);
        return false;
    }
    if (hdr.version != 1)
    {
        D3D11SW_ERROR("DXBCParseReflection: unsupported version {}", hdr.version);
        return false;
    }
    if (hdr.totalSize > len)
    {
        D3D11SW_ERROR("DXBCParseReflection: totalSize {} > buffer len {}", hdr.totalSize, len);
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
            ParseSignatureChunk(cdata, csz, out.outputs, chdr.fourCC == FOURCC_OSG5);
        }
        else if (chdr.fourCC == FOURCC_PCSG)
        {
            ParseSignatureChunk(cdata, csz, out.patchConstants);
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
        D3D11SW_ERROR("DXBCParse: DXBCParseReflection failed");
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
