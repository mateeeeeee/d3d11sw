#include <cstring>
#include "d3d9/translate/sm3_decoder.h"

namespace d3dsw {

static constexpr DWORD SM3_END_TOKEN = 0x0000FFFFu;

//Reg type is encoded across two disjoint fields:
//  bits[28:30] = low 3 bits
//  bits[11:12] = high 2 bits (already positioned to 3-4 of reg-type by SHIFT2=8)
static D3DSHADER_PARAM_REGISTER_TYPE DecodeRegType(DWORD tok)
{
    Uint32 lo = (tok & D3DSP_REGTYPE_MASK)  >> D3DSP_REGTYPE_SHIFT;   // bits 28-30 → bits 0-2
    Uint32 hi = (tok & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2;  // bits 11-12 → bits 3-4
    return static_cast<D3DSHADER_PARAM_REGISTER_TYPE>(lo | hi);
}

static void DecodeCommon(DWORD tok, SM3Operand& op)
{
    op.regType  = DecodeRegType(tok);
    op.regNum   = tok & D3DSP_REGNUM_MASK;
    op.relative = (tok & D3DSHADER_ADDRESSMODE_MASK) != 0;
}

//Destination parameter: bits[16:19]=writeMask, bits[20:23]=dstMod, bits[24:27]=dstShift (signed 4-bit).
static void DecodeDst(DWORD tok, SM3Operand& op)
{
    DecodeCommon(tok, op);
    op.kind      = SM3OperandKind::Destination;
    op.writeMask = static_cast<Uint8>((tok >> 16) & 0xF);
    op.dstMod    = static_cast<D3DSHADER_PARAM_DSTMOD_TYPE>(tok & D3DSP_DSTMOD_MASK);
    Uint8 s      = static_cast<Uint8>((tok >> D3DSP_DSTSHIFT_SHIFT) & 0xF);
    op.dstShift  = (s & 0x8) ? static_cast<Int8>(s | 0xF0)   // sign-extend 4-bit
                             : static_cast<Int8>(s);
}

//Source parameter: bits[16:23]=swizzle (2 bits × 4 components), bits[24:27]=srcMod.
static void DecodeSrc(DWORD tok, SM3Operand& op)
{
    DecodeCommon(tok, op);
    op.kind = SM3OperandKind::Source;
    Uint32 sw = (tok & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT;
    op.swizzle[0] = static_cast<Uint8>((sw >> 0) & 0x3);
    op.swizzle[1] = static_cast<Uint8>((sw >> 2) & 0x3);
    op.swizzle[2] = static_cast<Uint8>((sw >> 4) & 0x3);
    op.swizzle[3] = static_cast<Uint8>((sw >> 6) & 0x3);
    op.srcMod     = static_cast<D3DSHADER_PARAM_SRCMOD_TYPE>(tok & D3DSP_SRCMOD_MASK);
}

//The relative-address continuation token (SM2+): a source-shaped token that
//specifies which component of a0 / aL is used for indexing. 
static void DecodeRelative(DWORD tok, SM3Operand& op)
{
    op.relRegType = DecodeRegType(tok);
    op.relRegNum  = tok & D3DSP_REGNUM_MASK;
    op.relSwizzle = static_cast<Uint8>(((tok & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT) & 0x3);
}

static Bool OpcodeHasDst(D3DSHADER_INSTRUCTION_OPCODE_TYPE op)
{
    switch (op)
    {
        case D3DSIO_NOP:
        case D3DSIO_CALL:
        case D3DSIO_CALLNZ:
        case D3DSIO_LOOP:
        case D3DSIO_RET:
        case D3DSIO_ENDLOOP:
        case D3DSIO_LABEL:
        case D3DSIO_REP:
        case D3DSIO_ENDREP:
        case D3DSIO_IF:
        case D3DSIO_IFC:
        case D3DSIO_ELSE:
        case D3DSIO_ENDIF:
        case D3DSIO_BREAK:
        case D3DSIO_BREAKC:
        case D3DSIO_BREAKP:
        case D3DSIO_PHASE:
        case D3DSIO_TEXKILL:    // inspects src; no dst
            return false;
        default:
            return true;
    }
}

Bool SM3Decode(const DWORD* tokens, Usize numTokens, SM3DecodedShader& out)
{
    out = {};
    if (numTokens < 2)
    {
        return false;
    }

    // Version token: upper 16 bits = type tag (0xFFFE=VS, 0xFFFF=PS), lower 16 = major.minor.
    DWORD ver = tokens[0];
    Uint32 tag = ver >> 16;
    if (tag == 0xFFFEu)      { out.isPixelShader = false; }
    else if (tag == 0xFFFFu) { out.isPixelShader = true;  }
    else                     { return false; }

    out.majorVersion = static_cast<Uint8>((ver >> 8) & 0xFF);
    out.minorVersion = static_cast<Uint8>((ver >> 0) & 0xFF);
    if (out.majorVersion < 2 || out.majorVersion > 3)
    {
        return false;
    }

    Usize i = 1;
    while (i < numTokens)
    {
        DWORD opTok = tokens[i];
        DWORD opcode = opTok & D3DSI_OPCODE_MASK;
        if (opcode == D3DSIO_END || opTok == SM3_END_TOKEN)
        {
            return true;
        }

        if (opcode == D3DSIO_COMMENT)
        {
            Uint32 len = (opTok & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
            i += 1 + len;
            continue;
        }

        Uint32 instLen = (opTok & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;
        if (i + 1 + instLen > numTokens)
        {
            return false;
        }

        SM3Instruction instr;
        instr.op          = static_cast<D3DSHADER_INSTRUCTION_OPCODE_TYPE>(opcode);
        instr.controlBits = static_cast<Uint8>((opTok >> D3DSP_OPCODESPECIFICCONTROL_SHIFT) & 0xFF);
        instr.predicated  = (opTok & D3DSHADER_INSTRUCTION_PREDICATED) != 0;
        instr.coissue     = (opTok & D3DSI_COISSUE) != 0;

        const DWORD* p   = &tokens[i + 1];
        const DWORD* end = p + instLen;
        switch (opcode)
        {
            case D3DSIO_DCL:
            {
                if (instLen < 2) { return false; }
                DWORD usageTok = p[0];
                DWORD dstTok   = p[1];
                instr.declUsage       = static_cast<Uint8>((usageTok & D3DSP_DCL_USAGE_MASK)      >> D3DSP_DCL_USAGE_SHIFT);
                instr.declUsageIndex  = static_cast<Uint8>((usageTok & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT);
                instr.declSamplerType = static_cast<D3DSAMPLER_TEXTURE_TYPE>(usageTok & D3DSP_TEXTURETYPE_MASK);
                SM3Operand dst;
                DecodeDst(dstTok, dst);
                instr.operands.push_back(dst);
                break;
            }
            case D3DSIO_DEF:
            {
                if (instLen < 5) { return false; }
                SM3Operand dst;
                DecodeDst(p[0], dst);
                instr.operands.push_back(dst);
                for (int k = 0; k < 4; ++k)
                {
                    DWORD bits = p[1 + k];
                    std::memcpy(&instr.defFloat[k], &bits, sizeof(Float));
                }
                break;
            }
            case D3DSIO_DEFI:
            {
                if (instLen < 5) { return false; }
                SM3Operand dst;
                DecodeDst(p[0], dst);
                instr.operands.push_back(dst);
                for (int k = 0; k < 4; ++k)
                {
                    instr.defInt[k] = static_cast<Int32>(p[1 + k]);
                }
                break;
            }
            case D3DSIO_DEFB:
            {
                if (instLen < 2) { return false; }
                SM3Operand dst;
                DecodeDst(p[0], dst);
                instr.operands.push_back(dst);
                instr.defBool = p[1];
                break;
            }
            default:
            {
                const DWORD* q = p;
                const Bool hasDst = OpcodeHasDst(instr.op);
                if (hasDst)
                {
                    if (q >= end) { return false; }
                    SM3Operand dst;
                    DecodeDst(*q++, dst);
                    if (dst.relative && q < end)
                    {
                        DecodeRelative(*q++, dst);
                    }
                    instr.operands.push_back(dst);
                }
                while (q < end)
                {
                    SM3Operand src;
                    DecodeSrc(*q++, src);
                    if (src.relative && q < end)
                    {
                        DecodeRelative(*q++, src);
                    }
                    instr.operands.push_back(src);
                }
                break;
            }
        }

        out.instructions.push_back(std::move(instr));
        i += 1 + instLen;
    }
    return true;
}

}
