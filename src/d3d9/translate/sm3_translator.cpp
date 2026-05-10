// ---------------------------------------------------------------------------
// SM3 translator — the one-way boundary between D3D9 and the shared core.
//
// Rule: every quirk of SM3 (operand modifiers, opcode semantics, register-bank
// layout, sign conventions, etc.) must be lowered out of the shader HERE. The
// D3DSW_ParsedShader this file produces is SM4-native; by the time codegen
// sees it, there is no trace that the source was D3D9. If you find yourself
// wanting to add an `if (isD3D9) ...` branch in shader_codegen.cpp or the
// rasterizer, add the lowering here instead.
//
// Tripwire (must return empty on src/core):
//   grep -rE '\b(D3DSIO_|D3DSPR_|D3DSPSM_|D3DSPDM_|D3DDECLUSAGE_|D3DSTT_)\w+|#include.*d3d9' src/core/
// ---------------------------------------------------------------------------
#include "d3d9/translate/sm3_translator.h"
#include "d3d9/translate/sm3_decoder.h"
#include "core/shaders/sm4.h"
#include "core/common/common.h"
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <d3d11TokenizedProgramFormat.hpp>
#include <d3dcommon.h>

namespace d3dsw {

namespace {

// ---------------------------------------------------------------------------
// Constants table from DEF instructions — index → float4 payload.
// Looked up when translating D3DSPR_CONST sources; a hit becomes
// D3D10_SB_OPERAND_TYPE_IMMEDIATE32 (bypassing the CB).
// ---------------------------------------------------------------------------
struct DefConst
{
    Uint32 regNum;
    Float  value[4];
};

const DefConst* FindDef(const std::vector<DefConst>& defs, Uint32 regNum)
{
    for (const auto& d : defs)
    {
        if (d.regNum == regNum) { return &d; }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Signature semantic name mapping (D3DDECLUSAGE_* → SM4 signature name).
//
// D3D9 conflates input + output POSITION via the same D3DDECLUSAGE_POSITION
// enum, but downstream the two sides need different names: VS/PS *inputs*
// match the input layout's user semantic ("POSITION"), while VS *outputs*
// must surface as the SM4 system value "SV_Position" so the rasterizer
// picks them up. PS color outputs are emitted via D3DSPR_COLOROUT and named
// "SV_Target" elsewhere (no DCL for PS outputs in SM3).
// ---------------------------------------------------------------------------
const Char* UsageName(Uint8 usage, Bool isOutput)
{
    if (isOutput && usage == D3DDECLUSAGE_POSITION)
    {
        return "SV_Position";
    }
    switch (usage)
    {
    case D3DDECLUSAGE_POSITION:     return "POSITION";
    case D3DDECLUSAGE_BLENDWEIGHT:  return "BLENDWEIGHT";
    case D3DDECLUSAGE_BLENDINDICES: return "BLENDINDICES";
    case D3DDECLUSAGE_NORMAL:       return "NORMAL";
    case D3DDECLUSAGE_PSIZE:        return "PSIZE";
    case D3DDECLUSAGE_TEXCOORD:     return "TEXCOORD";
    case D3DDECLUSAGE_TANGENT:      return "TANGENT";
    case D3DDECLUSAGE_BINORMAL:     return "BINORMAL";
    case D3DDECLUSAGE_COLOR:        return "COLOR";
    case D3DDECLUSAGE_FOG:          return "FOG";
    case D3DDECLUSAGE_DEPTH:        return "DEPTH";
    default:                        return "UNKNOWN";
    }
}

Uint32 UsageSvType(Uint8 usage, Bool isOutput)
{
    // SV_Position is the only DECLUSAGE that doubles as an SM4 system value,
    // and only on the output side — VS/PS inputs are always user semantics.
    return (isOutput && usage == D3DDECLUSAGE_POSITION) ? D3D_NAME_POSITION : 0u;
}

Uint32 SamplerDimension(D3DSAMPLER_TEXTURE_TYPE stt)
{
    switch (stt)
    {
    case D3DSTT_2D:     return D3D_SRV_DIMENSION_TEXTURE2D;
    case D3DSTT_CUBE:   return D3D_SRV_DIMENSION_TEXTURECUBE;
    case D3DSTT_VOLUME: return D3D_SRV_DIMENSION_TEXTURE3D;
    case D3DSTT_UNKNOWN:
    default:            return D3D_SRV_DIMENSION_TEXTURE2D;
    }
}

// ---------------------------------------------------------------------------
// Operand conversion: SM3 → SM4.
// ---------------------------------------------------------------------------
Bool ConvertDst(const SM3Operand& in, SM4Operand& out, Uint32 predBase = 0)
{
    out = {};
    out.writeMask = in.writeMask ? in.writeMask : 0xF;
    out.indexDim  = 1;
    out.indices[0] = in.regNum;
    switch (in.regType)
    {
    case D3DSPR_TEMP:
        out.type = D3D10_SB_OPERAND_TYPE_TEMP;
        return true;
    case D3DSPR_OUTPUT:          // SM3 o#
        out.type = D3D10_SB_OPERAND_TYPE_OUTPUT;
        return true;
    case D3DSPR_COLOROUT:        // PS oC#
        out.type = D3D10_SB_OPERAND_TYPE_OUTPUT;
        return true;
    case D3DSPR_DEPTHOUT:        // PS oDepth
        out.type     = D3D10_SB_OPERAND_TYPE_OUTPUT_DEPTH;
        out.indexDim = 0;
        return true;
    case D3DSPR_PREDICATE:       // p# → mapped to high temp registers
        out.type       = D3D10_SB_OPERAND_TYPE_TEMP;
        out.indices[0] = predBase + in.regNum;
        return true;
    case D3DSPR_RASTOUT:         // SM2 oPos → output reg 8; oFog → output reg 11
        if (in.regNum == D3DSRO_POSITION) { out.indices[0] = 8; }
        else if (in.regNum == D3DSRO_FOG) { out.indices[0] = 11; }
        else { return false; }
        out.type = D3D10_SB_OPERAND_TYPE_OUTPUT;
        return true;
    case D3DSPR_ATTROUT:         // SM2 oD0/oD1 → output regs 9/10
        if (in.regNum > 1) { return false; }
        out.type       = D3D10_SB_OPERAND_TYPE_OUTPUT;
        out.indices[0] = 9 + in.regNum;
        return true;
    default:
        return false;
    }
}

Bool ConvertSrc(const SM3Operand& in, const std::vector<DefConst>& defs, SM4Operand& out,
                Uint32 predBase = 0)
{
    out = {};
    out.writeMask  = 0xF;
    out.swizzle[0] = in.swizzle[0];
    out.swizzle[1] = in.swizzle[1];
    out.swizzle[2] = in.swizzle[2];
    out.swizzle[3] = in.swizzle[3];
    out.negate   = (in.srcMod == D3DSPSM_NEG)    || (in.srcMod == D3DSPSM_ABSNEG);
    out.absolute = (in.srcMod == D3DSPSM_ABS)    || (in.srcMod == D3DSPSM_ABSNEG);
    if (in.srcMod != D3DSPSM_NONE &&
        in.srcMod != D3DSPSM_NEG  &&
        in.srcMod != D3DSPSM_ABS  &&
        in.srcMod != D3DSPSM_ABSNEG)
    {
        return false;            // BIAS/SIGN/COMP/X2/DZ/DW/NOT — MVP skip
    }

    switch (in.regType)
    {
    case D3DSPR_TEMP:
        out.type      = D3D10_SB_OPERAND_TYPE_TEMP;
        out.indexDim  = 1;
        out.indices[0] = in.regNum;
        return true;
    case D3DSPR_INPUT:
        out.type      = D3D10_SB_OPERAND_TYPE_INPUT;
        out.indexDim  = 1;
        out.indices[0] = in.regNum;
        return true;
    case D3DSPR_PREDICATE:      // p# → mapped to high temp registers
        out.type       = D3D10_SB_OPERAND_TYPE_TEMP;
        out.indexDim   = 1;
        out.indices[0] = predBase + in.regNum;
        return true;
    case D3DSPR_CONST:
    {
        if (const DefConst* d = FindDef(defs, in.regNum))
        {
            out.type     = D3D10_SB_OPERAND_TYPE_IMMEDIATE32;
            out.indexDim = 0;
            // DEF payload is already in dest-register order — apply the src
            // swizzle so shader codegen can treat IMMEDIATE32 as identity.
            for (Int i = 0; i < 4; ++i)
            {
                out.imm[i] = d->value[in.swizzle[i]];
            }
            out.swizzle[0] = 0;
            out.swizzle[1] = 1;
            out.swizzle[2] = 2;
            out.swizzle[3] = 3;
            return true;
        }
        out.type      = D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER;
        out.indexDim  = 2;
        out.indices[0] = 0;
        out.indices[1] = in.regNum;
        return true;
    }
    case D3DSPR_SAMPLER:
        out.type      = D3D10_SB_OPERAND_TYPE_SAMPLER;
        out.indexDim  = 1;
        out.indices[0] = in.regNum;
        return true;
    default:
        return false;            // CONSTINT/CONSTBOOL/MISCTYPE — not supported
    }
}

// Sampler operand, with its alternate identity as a "resource" (srv) slot so
// codegen can index res->srv[] by the same s# number the shader uses.
SM4Operand ResourceOperand(Uint32 regNum)
{
    SM4Operand op{};
    op.type       = D3D10_SB_OPERAND_TYPE_RESOURCE;
    op.indexDim   = 1;
    op.indices[0] = regNum;
    op.writeMask  = 0xF;
    op.swizzle[0] = 0; op.swizzle[1] = 1; op.swizzle[2] = 2; op.swizzle[3] = 3;
    return op;
}

// ---------------------------------------------------------------------------
// Instruction emission helpers.
// ---------------------------------------------------------------------------
SM4Instruction MakeInstr(D3D10_SB_OPCODE_TYPE op)
{
    SM4Instruction ins{};
    ins.op          = op;
    ins.saturate    = false;
    ins.testNonZero = true;
    return ins;
}

Bool TranslateSimple(D3D10_SB_OPCODE_TYPE sm4op, Uint srcCount,
                     const SM3Instruction& sm3,
                     const std::vector<DefConst>& defs,
                     D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 1 + srcCount) { return false; }

    SM4Instruction ins = MakeInstr(sm4op);
    ins.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);

    SM4Operand dst;
    if (!ConvertDst(sm3.operands[0], dst)) { return false; }
    ins.operands.push_back(dst);

    for (Uint i = 0; i < srcCount; ++i)
    {
        SM4Operand src;
        if (!ConvertSrc(sm3.operands[1 + i], defs, src)) { return false; }
        ins.operands.push_back(src);
    }
    out.instrs.push_back(std::move(ins));
    return true;
}

// Matrix macros: MxA × NxB → N dot products.
// `dst.<comp_i>` = dot(src0, src1_offset_by_i) for i in 0..rowCount-1.
Bool TranslateMatrixMul(D3D10_SB_OPCODE_TYPE dotOp, Uint rowCount,
                        const SM3Instruction& sm3,
                        const std::vector<DefConst>& defs,
                        D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 3) { return false; }

    SM4Operand src0;
    if (!ConvertSrc(sm3.operands[1], defs, src0)) { return false; }

    const SM3Operand& src1base = sm3.operands[2];
    const Uint8 dstMask = sm3.operands[0].writeMask ? sm3.operands[0].writeMask : 0xF;

    for (Uint r = 0; r < rowCount; ++r)
    {
        SM4Instruction ins = MakeInstr(dotOp);
        ins.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);

        SM4Operand dst;
        if (!ConvertDst(sm3.operands[0], dst)) { return false; }
        dst.writeMask = static_cast<Uint8>(dstMask & (1u << r));
        if (dst.writeMask == 0) { continue; }
        ins.operands.push_back(dst);
        ins.operands.push_back(src0);

        SM3Operand rowSrc = src1base;
        rowSrc.regNum    = src1base.regNum + r;
        SM4Operand src1;
        if (!ConvertSrc(rowSrc, defs, src1)) { return false; }
        ins.operands.push_back(src1);

        out.instrs.push_back(std::move(ins));
    }
    return true;
}

// LRP dst, s, a, b → dst = s*(a - b) + b, implemented as:
//   ADD tmp, a, -b
//   MAD dst, s, tmp, b
// Allocates a fresh temp one past the max r# referenced so far.
Bool TranslateLRP(const SM3Instruction& sm3,
                  const std::vector<DefConst>& defs,
                  Uint32& numTemps,
                  D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 4) { return false; }

    SM4Operand dst, s, a, b;
    if (!ConvertDst(sm3.operands[0], dst))                 { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, s))             { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, a))             { return false; }
    if (!ConvertSrc(sm3.operands[3], defs, b))             { return false; }

    Uint32 tmpReg = numTemps++;

    SM4Operand tmpDst{};
    tmpDst.type       = D3D10_SB_OPERAND_TYPE_TEMP;
    tmpDst.indexDim   = 1;
    tmpDst.indices[0] = tmpReg;
    tmpDst.writeMask  = 0xF;

    SM4Operand tmpSrc{};
    tmpSrc.type       = D3D10_SB_OPERAND_TYPE_TEMP;
    tmpSrc.indexDim   = 1;
    tmpSrc.indices[0] = tmpReg;
    tmpSrc.writeMask  = 0xF;
    tmpSrc.swizzle[0] = 0; tmpSrc.swizzle[1] = 1; tmpSrc.swizzle[2] = 2; tmpSrc.swizzle[3] = 3;

    SM4Operand bNeg = b;
    bNeg.negate = !bNeg.negate;

    SM4Instruction add = MakeInstr(D3D10_SB_OPCODE_ADD);
    add.operands = { tmpDst, a, bNeg };
    out.instrs.push_back(std::move(add));

    SM4Instruction mad = MakeInstr(D3D10_SB_OPCODE_MAD);
    mad.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    mad.operands = { dst, s, tmpSrc, b };
    out.instrs.push_back(std::move(mad));
    return true;
}

// Helper: build a temp operand (dst form with full write mask).
static SM4Operand TempDst(Uint32 reg)
{
    SM4Operand o{};
    o.type       = D3D10_SB_OPERAND_TYPE_TEMP;
    o.indexDim   = 1;
    o.indices[0] = reg;
    o.writeMask  = 0xF;
    return o;
}
static SM4Operand TempSrc(Uint32 reg)
{
    SM4Operand o{};
    o.type       = D3D10_SB_OPERAND_TYPE_TEMP;
    o.indexDim   = 1;
    o.indices[0] = reg;
    o.swizzle[0] = 0; o.swizzle[1] = 1; o.swizzle[2] = 2; o.swizzle[3] = 3;
    return o;
}
static SM4Operand ImmF4(Float x, Float y, Float z, Float w)
{
    SM4Operand o{};
    o.type        = D3D10_SB_OPERAND_TYPE_IMMEDIATE32;
    o.indexDim    = 0;
    o.imm[0] = x;
    o.imm[1] = y;
    o.imm[2] = z;
    o.imm[3] = w;
    o.swizzle[0] = 0; o.swizzle[1] = 1; o.swizzle[2] = 2; o.swizzle[3] = 3;
    return o;
}

// SLT dst, src0, src1  →  dst = (src0 < src1) ? 1.0 : 0.0
//   LT  tmp, src0, src1     (tmp = 0xFFFFFFFF or 0x0 per component)
//   MOVC dst, tmp, 1.0, 0.0
Bool TranslateSLT(const SM3Instruction& sm3, const std::vector<DefConst>& defs,
                  Uint32& numTemps, D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 3) { return false; }
    SM4Operand dst, a, b;
    if (!ConvertDst(sm3.operands[0], dst))     { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, a)) { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, b)) { return false; }

    Uint32 tmp = numTemps++;
    SM4Instruction lt = MakeInstr(D3D10_SB_OPCODE_LT);
    lt.operands = { TempDst(tmp), a, b };
    out.instrs.push_back(std::move(lt));

    SM4Instruction movc = MakeInstr(D3D10_SB_OPCODE_MOVC);
    movc.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    movc.operands = { dst, TempSrc(tmp), ImmF4(1.f, 1.f, 1.f, 1.f), ImmF4(0.f, 0.f, 0.f, 0.f) };
    out.instrs.push_back(std::move(movc));
    return true;
}

// SGE dst, src0, src1  →  dst = (src0 >= src1) ? 1.0 : 0.0
//   GE  tmp, src0, src1
//   MOVC dst, tmp, 1.0, 0.0
Bool TranslateSGE(const SM3Instruction& sm3, const std::vector<DefConst>& defs,
                  Uint32& numTemps, D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 3) { return false; }
    SM4Operand dst, a, b;
    if (!ConvertDst(sm3.operands[0], dst))     { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, a)) { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, b)) { return false; }

    Uint32 tmp = numTemps++;
    SM4Instruction ge = MakeInstr(D3D10_SB_OPCODE_GE);
    ge.operands = { TempDst(tmp), a, b };
    out.instrs.push_back(std::move(ge));

    SM4Instruction movc = MakeInstr(D3D10_SB_OPCODE_MOVC);
    movc.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    movc.operands = { dst, TempSrc(tmp), ImmF4(1.f, 1.f, 1.f, 1.f), ImmF4(0.f, 0.f, 0.f, 0.f) };
    out.instrs.push_back(std::move(movc));
    return true;
}

// CMP dst, src0, src1, src2  →  dst = (src0 >= 0) ? src1 : src2
//   GE  tmp, src0, 0.0
//   MOVC dst, tmp, src1, src2
Bool TranslateCMP(const SM3Instruction& sm3, const std::vector<DefConst>& defs,
                  Uint32& numTemps, D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 4) { return false; }
    SM4Operand dst, s0, s1, s2;
    if (!ConvertDst(sm3.operands[0], dst))     { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, s0)) { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, s1)) { return false; }
    if (!ConvertSrc(sm3.operands[3], defs, s2)) { return false; }

    Uint32 tmp = numTemps++;
    SM4Instruction ge = MakeInstr(D3D10_SB_OPCODE_GE);
    ge.operands = { TempDst(tmp), s0, ImmF4(0.f, 0.f, 0.f, 0.f) };
    out.instrs.push_back(std::move(ge));

    SM4Instruction movc = MakeInstr(D3D10_SB_OPCODE_MOVC);
    movc.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    movc.operands = { dst, TempSrc(tmp), s1, s2 };
    out.instrs.push_back(std::move(movc));
    return true;
}

// NRM dst, src  →  dst = normalize(src.xyz), w=0
//   DP3 tmp.x, src, src
//   RSQ tmp.x, tmp.x
//   MUL dst, src, tmp.xxxx
Bool TranslateNRM(const SM3Instruction& sm3, const std::vector<DefConst>& defs,
                  Uint32& numTemps, D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 2) { return false; }
    SM4Operand dst, src;
    if (!ConvertDst(sm3.operands[0], dst))     { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, src)) { return false; }

    Uint32 tmp = numTemps++;

    // DP3 tmp.x, src, src
    SM4Operand tmpDstX  = TempDst(tmp); tmpDstX.writeMask = 0x1;   // .x only
    SM4Operand tmpSrcX  = TempSrc(tmp);
    tmpSrcX.swizzle[0] = tmpSrcX.swizzle[1] = tmpSrcX.swizzle[2] = tmpSrcX.swizzle[3] = 0; // .xxxx

    SM4Instruction dp3 = MakeInstr(D3D10_SB_OPCODE_DP3);
    dp3.operands = { tmpDstX, src, src };
    out.instrs.push_back(std::move(dp3));

    // RSQ tmp.x, tmp.x
    SM4Operand tmpSrcScalar = TempSrc(tmp);
    tmpSrcScalar.swizzle[0] = tmpSrcScalar.swizzle[1] = tmpSrcScalar.swizzle[2] = tmpSrcScalar.swizzle[3] = 0;
    SM4Instruction rsq = MakeInstr(D3D10_SB_OPCODE_RSQ);
    rsq.operands = { tmpDstX, tmpSrcScalar };
    out.instrs.push_back(std::move(rsq));

    // MUL dst, src, tmp.xxxx
    SM4Instruction mul = MakeInstr(D3D10_SB_OPCODE_MUL);
    mul.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    mul.operands = { dst, src, tmpSrcX };
    out.instrs.push_back(std::move(mul));
    return true;
}

// POW dst, src0.x, src1.x  →  dst.xyzw = pow(|src0.x|, src1.x)
//   LOG tmp.x, |src0.x|         (log2)
//   MUL tmp.x, tmp.x, src1.x
//   EXP dst, tmp.xxxx            (exp2)
Bool TranslatePOW(const SM3Instruction& sm3, const std::vector<DefConst>& defs,
                  Uint32& numTemps, D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 3) { return false; }
    SM4Operand dst, s0, s1;
    if (!ConvertDst(sm3.operands[0], dst))     { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, s0)) { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, s1)) { return false; }

    // Scalar-select src components (POW only uses .x of each source in SM3).
    SM4Operand s0x = s0; s0x.swizzle[0] = s0x.swizzle[1] = s0x.swizzle[2] = s0x.swizzle[3] = s0.swizzle[0];
    SM4Operand s1x = s1; s1x.swizzle[0] = s1x.swizzle[1] = s1x.swizzle[2] = s1x.swizzle[3] = s1.swizzle[0];
    s0x.absolute = true;  // D3D9 POW uses |src0|

    Uint32 tmp = numTemps++;
    SM4Operand tmpDstX = TempDst(tmp); tmpDstX.writeMask = 0x1;
    SM4Operand tmpSrcX = TempSrc(tmp); tmpSrcX.swizzle[0] = tmpSrcX.swizzle[1] = tmpSrcX.swizzle[2] = tmpSrcX.swizzle[3] = 0;

    SM4Instruction log2 = MakeInstr(D3D10_SB_OPCODE_LOG);
    log2.operands = { tmpDstX, s0x };
    out.instrs.push_back(std::move(log2));

    SM4Instruction mul = MakeInstr(D3D10_SB_OPCODE_MUL);
    mul.operands = { tmpDstX, tmpSrcX, s1x };
    out.instrs.push_back(std::move(mul));

    SM4Instruction exp2 = MakeInstr(D3D10_SB_OPCODE_EXP);
    exp2.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    exp2.operands = { dst, tmpSrcX };
    out.instrs.push_back(std::move(exp2));
    return true;
}

// SINCOS dst, src0 [, src1_macro, src2_macro]
//   src1/src2 are D3D8/9 macro constants, ignored in SM3.0 (shader model handles
//   the Taylor expansion internally). SM4 SINCOS is a native instruction.
//   SM3 SINCOS: dst.x = cos(src0.x), dst.y = sin(src0.x).
//   SM4 SINCOS: dst0 = sin, dst1 = cos. Emit two SINCOS or one with both outputs.
Bool TranslateSINCOS(const SM3Instruction& sm3, const std::vector<DefConst>& defs,
                     Uint32& numTemps, D3DSW_ParsedShader& out)
{
    // Operand count: 2 (sm3_0 bare) or 4 (SM2/SM3 with macro constants — ignore extras)
    if (sm3.operands.size() < 2) { return false; }
    SM4Operand dst, src;
    if (!ConvertDst(sm3.operands[0], dst))     { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, src)) { return false; }

    // Scalar source: use only .x of src
    SM4Operand srcX = src; srcX.swizzle[0] = srcX.swizzle[1] = srcX.swizzle[2] = srcX.swizzle[3] = src.swizzle[0];

    // Allocate two scalar temps: one for sin, one for cos.
    Uint32 tmpSin = numTemps++;
    Uint32 tmpCos = numTemps++;

    SM4Operand sinDst = TempDst(tmpSin); sinDst.writeMask = 0x1;
    SM4Operand cosDst = TempDst(tmpCos); cosDst.writeMask = 0x1;
    SM4Operand sinSrc = TempSrc(tmpSin); sinSrc.swizzle[0] = sinSrc.swizzle[1] = sinSrc.swizzle[2] = sinSrc.swizzle[3] = 0;
    SM4Operand cosSrc = TempSrc(tmpCos); cosSrc.swizzle[0] = cosSrc.swizzle[1] = cosSrc.swizzle[2] = cosSrc.swizzle[3] = 0;

    // SINCOS sinDst, cosDst, srcX
    SM4Instruction sc = MakeInstr(D3D10_SB_OPCODE_SINCOS);
    sc.operands = { sinDst, cosDst, srcX };
    out.instrs.push_back(std::move(sc));

    // Write cos to dst.x, sin to dst.y (SM3 SINCOS convention).
    Uint8 origMask = dst.writeMask;
    if (origMask & 0x1)  // dst.x = cos
    {
        SM4Operand dstX = dst; dstX.writeMask = 0x1;
        SM4Instruction mov = MakeInstr(D3D10_SB_OPCODE_MOV);
        mov.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
        mov.operands = { dstX, cosSrc };
        out.instrs.push_back(std::move(mov));
    }
    if (origMask & 0x2)  // dst.y = sin
    {
        SM4Operand dstY = dst; dstY.writeMask = 0x2;
        SM4Instruction mov = MakeInstr(D3D10_SB_OPCODE_MOV);
        mov.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
        mov.operands = { dstY, sinSrc };
        out.instrs.push_back(std::move(mov));
    }
    return true;
}

// DST dst, src0, src1  →  dst = (1, src0.y*src1.y, src0.z, src1.w)
//   MOV dst.x, 1.0
//   MUL dst.y, src0.y, src1.y
//   MOV dst.z, src0.z
//   MOV dst.w, src1.w
Bool TranslateDST(const SM3Instruction& sm3, const std::vector<DefConst>& defs,
                  D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 3) { return false; }
    SM4Operand dst, s0, s1;
    if (!ConvertDst(sm3.operands[0], dst))      { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, s0)) { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, s1)) { return false; }

    auto Scalar = [](SM4Operand o, Uint8 c) {
        o.swizzle[0] = o.swizzle[1] = o.swizzle[2] = o.swizzle[3] = c; return o;
    };
    auto DstComp = [&](Uint8 mask) { SM4Operand d = dst; d.writeMask = mask; return d; };

    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MOV); i.operands = { DstComp(0x1), ImmF4(1.f,1.f,1.f,1.f) }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MUL); i.operands = { DstComp(0x2), Scalar(s0,s0.swizzle[1]), Scalar(s1,s1.swizzle[1]) }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MOV); i.operands = { DstComp(0x4), Scalar(s0,s0.swizzle[2]) }; out.instrs.push_back(i); }
    SM4Instruction last = MakeInstr(D3D10_SB_OPCODE_MOV);
    last.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    last.operands = { DstComp(0x8), Scalar(s1,s1.swizzle[3]) };
    out.instrs.push_back(std::move(last));
    return true;
}

// CRS dst, src0, src1  →  cross product (xyz only)
//   MUL tmp,   src0.yzxw, src1.zxyw
//   MAD dst, -src0.zxyw, src1.yzxw, tmp
Bool TranslateCRS(const SM3Instruction& sm3, const std::vector<DefConst>& defs,
                  Uint32& numTemps, D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 3) { return false; }
    SM4Operand dst, s0, s1;
    if (!ConvertDst(sm3.operands[0], dst))      { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, s0)) { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, s1)) { return false; }

    auto Swz3 = [](SM4Operand o, Uint8 a, Uint8 b, Uint8 c) {
        o.swizzle[0]=a; o.swizzle[1]=b; o.swizzle[2]=c; o.swizzle[3]=c; return o;
    };

    Uint32 tmp = numTemps++;
    SM4Operand tmpDst = TempDst(tmp); tmpDst.writeMask = 0x7;
    SM4Operand tmpSrc = TempSrc(tmp);

    SM4Instruction mul = MakeInstr(D3D10_SB_OPCODE_MUL);
    mul.operands = { tmpDst, Swz3(s0,s0.swizzle[1],s0.swizzle[2],s0.swizzle[0]),
                              Swz3(s1,s1.swizzle[2],s1.swizzle[0],s1.swizzle[1]) };
    out.instrs.push_back(std::move(mul));

    SM4Operand s0neg = Swz3(s0,s0.swizzle[2],s0.swizzle[0],s0.swizzle[1]); s0neg.negate = !s0neg.negate;
    SM4Instruction mad = MakeInstr(D3D10_SB_OPCODE_MAD);
    mad.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    mad.operands = { dst, s0neg, Swz3(s1,s1.swizzle[1],s1.swizzle[2],s1.swizzle[0]), tmpSrc };
    out.instrs.push_back(std::move(mad));
    return true;
}

// DP2ADD dst, src0, src1, src2  →  dst = dot(src0.xy, src1.xy) + src2.x
//   MUL tmp.x, src0.x, src1.x
//   MAD tmp.x, src0.y, src1.y, tmp.x
//   ADD dst,   tmp.xxxx, src2.xxxx
Bool TranslateDP2ADD(const SM3Instruction& sm3, const std::vector<DefConst>& defs,
                     Uint32& numTemps, D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 4) { return false; }
    SM4Operand dst, s0, s1, s2;
    if (!ConvertDst(sm3.operands[0], dst))      { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, s0)) { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, s1)) { return false; }
    if (!ConvertSrc(sm3.operands[3], defs, s2)) { return false; }

    auto X = [](SM4Operand o) { o.swizzle[0]=o.swizzle[1]=o.swizzle[2]=o.swizzle[3]=o.swizzle[0]; return o; };
    auto Y = [](SM4Operand o) { o.swizzle[0]=o.swizzle[1]=o.swizzle[2]=o.swizzle[3]=o.swizzle[1]; return o; };

    Uint32 tmp = numTemps++;
    SM4Operand tmpDstX = TempDst(tmp); tmpDstX.writeMask = 0x1;
    SM4Operand tmpSrcX = TempSrc(tmp); tmpSrcX.swizzle[0]=tmpSrcX.swizzle[1]=tmpSrcX.swizzle[2]=tmpSrcX.swizzle[3]=0;

    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MUL); i.operands = { tmpDstX, X(s0), X(s1) }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MAD); i.operands = { tmpDstX, Y(s0), Y(s1), tmpSrcX }; out.instrs.push_back(i); }
    SM4Instruction add = MakeInstr(D3D10_SB_OPCODE_ADD);
    add.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    add.operands = { dst, tmpSrcX, X(s2) };
    out.instrs.push_back(std::move(add));
    return true;
}

// SGN dst, src0, src1(tmp), src2(tmp)  →  dst = sign(src0): -1/0/1
//   LT tmp1, 0.0, src0   →  tmp1 = src0 > 0
//   MOVC pos, tmp1, 1.0, 0.0
//   LT tmp1, src0, 0.0   →  tmp1 = src0 < 0
//   MOVC neg, tmp1, -1.0, 0.0
//   ADD dst, pos, neg
// src1/src2 are SM3 scratch operands — we allocate our own temps instead.
Bool TranslateSGN(const SM3Instruction& sm3, const std::vector<DefConst>& defs,
                  Uint32& numTemps, D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 4) { return false; }
    SM4Operand dst, s0;
    if (!ConvertDst(sm3.operands[0], dst))      { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, s0)) { return false; }

    Uint32 tCmp = numTemps++;
    Uint32 tPos = numTemps++;
    Uint32 tNeg = numTemps++;

    SM4Operand zero = ImmF4(0.f, 0.f, 0.f, 0.f);
    SM4Operand one  = ImmF4(1.f, 1.f, 1.f, 1.f);
    SM4Operand mone = ImmF4(-1.f,-1.f,-1.f,-1.f);

    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_LT);   i.operands = { TempDst(tCmp), zero, s0 };                          out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MOVC); i.operands = { TempDst(tPos), TempSrc(tCmp), one,  zero };          out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_LT);   i.operands = { TempDst(tCmp), s0, zero };                          out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MOVC); i.operands = { TempDst(tNeg), TempSrc(tCmp), mone, zero };          out.instrs.push_back(i); }
    SM4Instruction add = MakeInstr(D3D10_SB_OPCODE_ADD);
    add.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    add.operands = { dst, TempSrc(tPos), TempSrc(tNeg) };
    out.instrs.push_back(std::move(add));
    return true;
}

// LIT dst, src0  →  dst = (1, max(src0.x,0), src0.x>0 ? max(src0.y,0)^clamp(src0.w,-128,128) : 0, 1)
//   MAX t.y, src0.y, 0
//   MAX t.x, src0.w, -128
//   MIN t.x, t.x, 128
//   LOG t.y, t.y            (log2; -inf if t.y==0)
//   MUL t.y, t.y, t.x       (w * log2(nh))
//   EXP t.y, t.y             (nh^w)
//   GE  t.x, src0.x, 0      (nl >= 0?)
//   MOVC dst.z, t.x, t.y, 0
//   MOV dst.x, 1.0 ; MAX dst.y, src0.x, 0 ; MOV dst.w, 1.0
Bool TranslateLIT(const SM3Instruction& sm3, const std::vector<DefConst>& defs,
                  Uint32& numTemps, D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 2) { return false; }
    SM4Operand dst, s0;
    if (!ConvertDst(sm3.operands[0], dst))      { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, s0)) { return false; }

    auto X = [](SM4Operand o) { o.swizzle[0]=o.swizzle[1]=o.swizzle[2]=o.swizzle[3]=o.swizzle[0]; return o; };
    auto Y = [](SM4Operand o) { o.swizzle[0]=o.swizzle[1]=o.swizzle[2]=o.swizzle[3]=o.swizzle[1]; return o; };
    auto W = [](SM4Operand o) { o.swizzle[0]=o.swizzle[1]=o.swizzle[2]=o.swizzle[3]=o.swizzle[3]; return o; };
    auto DstComp = [&](Uint8 mask) { SM4Operand d = dst; d.writeMask = mask; return d; };

    Uint32 t = numTemps++;
    SM4Operand tx = TempDst(t); tx.writeMask = 0x1;
    SM4Operand ty = TempDst(t); ty.writeMask = 0x2;
    SM4Operand txs = TempSrc(t); txs.swizzle[0]=txs.swizzle[1]=txs.swizzle[2]=txs.swizzle[3]=0;
    SM4Operand tys = TempSrc(t); tys.swizzle[0]=tys.swizzle[1]=tys.swizzle[2]=tys.swizzle[3]=1;

    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MAX); i.operands = { ty, Y(s0), ImmF4(0,0,0,0) }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MAX); i.operands = { tx, W(s0), ImmF4(-128,-128,-128,-128) }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MIN); i.operands = { tx, txs,  ImmF4(128,128,128,128) }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_LOG); i.operands = { ty, tys }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MUL); i.operands = { ty, tys, txs }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_EXP); i.operands = { ty, tys }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_GE);  i.operands = { tx, X(s0), ImmF4(0,0,0,0) }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MOVC);i.operands = { DstComp(0x4), txs, tys, ImmF4(0,0,0,0) }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MOV); i.operands = { DstComp(0x1), ImmF4(1,1,1,1) }; out.instrs.push_back(i); }
    { SM4Instruction i = MakeInstr(D3D10_SB_OPCODE_MAX); i.operands = { DstComp(0x2), X(s0), ImmF4(0,0,0,0) }; out.instrs.push_back(i); }
    SM4Instruction last = MakeInstr(D3D10_SB_OPCODE_MOV);
    last.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    last.operands = { DstComp(0x8), ImmF4(1,1,1,1) };
    out.instrs.push_back(std::move(last));
    return true;
}

// TEXLDD dst, src0(uv), src1(sampler), src2(ddx), src3(ddy)  →  SAMPLE_D
Bool TranslateTexldd(const SM3Instruction& sm3,
                     const std::vector<DefConst>& defs,
                     D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 5)                  { return false; }
    if (sm3.operands[1].regType != D3DSPR_SAMPLER) { return false; }
    SM4Operand dst, uv, sampler, ddx, ddy;
    if (!ConvertDst(sm3.operands[0], dst))         { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, uv))    { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, sampler)){ return false; }
    if (!ConvertSrc(sm3.operands[3], defs, ddx))   { return false; }
    if (!ConvertSrc(sm3.operands[4], defs, ddy))   { return false; }
    SM4Instruction ins = MakeInstr(D3D10_SB_OPCODE_SAMPLE_D);
    ins.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    ins.operands = { dst, uv, ResourceOperand(sm3.operands[1].regNum), sampler, ddx, ddy };
    out.instrs.push_back(std::move(ins));
    return true;
}

// TEXLD dst, src0 (uv), src1 (sampler)
Bool TranslateTexld(const SM3Instruction& sm3,
                    const std::vector<DefConst>& defs,
                    D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 3)                         { return false; }
    if (sm3.operands[2].regType != D3DSPR_SAMPLER)        { return false; }

    SM4Instruction ins = MakeInstr(D3D10_SB_OPCODE_SAMPLE);
    ins.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);

    SM4Operand dst, uv, sampler;
    if (!ConvertDst(sm3.operands[0], dst))                { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, uv))           { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, sampler))      { return false; }

    ins.operands.push_back(dst);
    ins.operands.push_back(uv);
    ins.operands.push_back(ResourceOperand(sm3.operands[2].regNum));
    ins.operands.push_back(sampler);
    out.instrs.push_back(std::move(ins));
    return true;
}

// TEXLDB dst, src0, src1(sampler)  →  SAMPLE_B dst, uv, resource, sampler, bias
// src0.xy = uv coords, src0.w = LOD bias.
Bool TranslateTexldb(const SM3Instruction& sm3,
                     const std::vector<DefConst>& defs,
                     D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 3)                  { return false; }
    if (sm3.operands[2].regType != D3DSPR_SAMPLER) { return false; }

    SM4Operand dst, uv, sampler;
    if (!ConvertDst(sm3.operands[0], dst))      { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, uv)) { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, sampler)) { return false; }

    // Bias scalar: src0.w (select .w component into all slots)
    SM4Operand bias = uv;
    bias.swizzle[0] = bias.swizzle[1] = bias.swizzle[2] = bias.swizzle[3] = uv.swizzle[3];

    SM4Instruction ins = MakeInstr(D3D10_SB_OPCODE_SAMPLE_B);
    ins.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    ins.operands = { dst, uv, ResourceOperand(sm3.operands[2].regNum), sampler, bias };
    out.instrs.push_back(std::move(ins));
    return true;
}

// TEXLDL dst, src0, src1(sampler)  →  SAMPLE_L dst, uv, resource, sampler, lod
// src0.xy = uv coords, src0.w = explicit LOD.
Bool TranslateTexldl(const SM3Instruction& sm3,
                     const std::vector<DefConst>& defs,
                     D3DSW_ParsedShader& out)
{
    if (sm3.operands.size() != 3)                  { return false; }
    if (sm3.operands[2].regType != D3DSPR_SAMPLER) { return false; }

    SM4Operand dst, uv, sampler;
    if (!ConvertDst(sm3.operands[0], dst))      { return false; }
    if (!ConvertSrc(sm3.operands[1], defs, uv)) { return false; }
    if (!ConvertSrc(sm3.operands[2], defs, sampler)) { return false; }

    // LOD scalar: src0.w
    SM4Operand lod = uv;
    lod.swizzle[0] = lod.swizzle[1] = lod.swizzle[2] = lod.swizzle[3] = uv.swizzle[3];

    SM4Instruction ins = MakeInstr(D3D10_SB_OPCODE_SAMPLE_L);
    ins.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
    ins.operands = { dst, uv, ResourceOperand(sm3.operands[2].regNum), sampler, lod };
    out.instrs.push_back(std::move(ins));
    return true;
}

// ---------------------------------------------------------------------------
// Temp tracking: scan every operand for max TEMP index to size numTemps.
// ---------------------------------------------------------------------------
void TrackTemps(const SM3Operand& op, Uint32& numTemps)
{
    if (op.regType == D3DSPR_TEMP && op.regNum + 1 > numTemps)
    {
        numTemps = op.regNum + 1;
    }
}

// Lowest set bit → component index (0..3). Used to pick the semanticIndex for
// oC# writes, though in practice the write mask is 0xF on all color outputs.
Uint32 FirstComponent(Uint8 mask)
{
    for (Int i = 0; i < 4; ++i)
    {
        if (mask & (1u << i)) { return static_cast<Uint32>(i); }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Signature construction.
// ---------------------------------------------------------------------------
void AppendSignatureEntry(std::vector<D3DSW_ShaderSignatureElement>& sig,
                          const Char* name, Uint32 semIndex, Uint32 reg,
                          Uint8 mask, Uint32 svType)
{
    for (const auto& e : sig)
    {
        if (e.reg == reg && std::strncmp(e.name, name, sizeof(e.name)) == 0 &&
            e.semanticIndex == semIndex)
        {
            return;
        }
    }
    D3DSW_ShaderSignatureElement elem{};
    std::strncpy(elem.name, name, sizeof(elem.name) - 1);
    elem.name[sizeof(elem.name) - 1] = '\0';
    elem.semanticIndex = semIndex;
    elem.reg           = reg;
    elem.mask          = mask;
    elem.svType        = svType;
    sig.push_back(elem);
}

void BuildSignatures(const SM3DecodedShader& in, D3DSW_ParsedShader& out)
{
    const Bool isPS = in.isPixelShader;

    // DCL-declared inputs/outputs and sampler bindings.
    for (const auto& ins : in.instructions)
    {
        if (ins.op != D3DSIO_DCL || ins.operands.empty()) { continue; }
        const SM3Operand& op = ins.operands[0];

        if (op.regType == D3DSPR_INPUT)
        {
            AppendSignatureEntry(out.inputs, UsageName(ins.declUsage, false),
                                 ins.declUsageIndex, op.regNum, 0xF,
                                 UsageSvType(ins.declUsage, false));
        }
        else if (op.regType == D3DSPR_OUTPUT)
        {
            AppendSignatureEntry(out.outputs, UsageName(ins.declUsage, true),
                                 ins.declUsageIndex, op.regNum, 0xF,
                                 UsageSvType(ins.declUsage, true));
        }
        else if (op.regType == D3DSPR_SAMPLER)
        {
            D3DSW_TexBinding tb{};
            tb.slot      = op.regNum;
            tb.dimension = SamplerDimension(ins.declSamplerType);
            out.textures.push_back(tb);
        }
    }

    // SM2 VS: outputs are never DCL'd — infer all from destination register writes.
    // oT0–oT7 (OUTPUT[0–7]) → regs 0–7; oPos (RASTOUT[0]) → reg 8;
    // oD0/oD1 (ATTROUT[0/1]) → regs 9/10.
    if (!isPS && in.majorVersion == 2)
    {
        Bool hasOPos  = false;
        Bool hasOFog  = false;
        Bool hasOD[2] = {};
        Bool hasOT[8] = {};
        for (const auto& ins : in.instructions)
        {
            if (ins.operands.empty()) { continue; }
            const SM3Operand& d = ins.operands[0];
            if (d.kind != SM3OperandKind::Destination) { continue; }
            if      (d.regType == D3DSPR_RASTOUT && d.regNum == D3DSRO_POSITION) { hasOPos       = true; }
            else if (d.regType == D3DSPR_RASTOUT && d.regNum == D3DSRO_FOG)      { hasOFog       = true; }
            else if (d.regType == D3DSPR_ATTROUT  && d.regNum < 2)               { hasOD[d.regNum] = true; }
            else if (d.regType == D3DSPR_OUTPUT   && d.regNum < 8)               { hasOT[d.regNum] = true; }
        }
        if (hasOPos)
            AppendSignatureEntry(out.outputs, "SV_Position", 0, 8, 0xF, D3D_NAME_POSITION);
        if (hasOFog)
            AppendSignatureEntry(out.outputs, "FOG", 0, 11, 0x1, 0u);
        for (Uint32 i = 0; i < 2; ++i)
            if (hasOD[i])
                AppendSignatureEntry(out.outputs, "COLOR", i, 9 + i, 0xF, 0u);
        for (Uint32 i = 0; i < 8; ++i)
            if (hasOT[i])
                AppendSignatureEntry(out.outputs, "TEXCOORD", i, i, 0xF, 0u);
    }

    // PS color outputs: inferred from writes to D3DSPR_COLOROUT (no DCL in SM3).
    if (isPS)
    {
        for (const auto& ins : in.instructions)
        {
            if (ins.operands.empty()) { continue; }
            const SM3Operand& d = ins.operands[0];
            if (d.kind != SM3OperandKind::Destination) { continue; }
            if (d.regType == D3DSPR_COLOROUT)
            {
                AppendSignatureEntry(out.outputs, "SV_Target", d.regNum,
                                     d.regNum, 0xF, D3D_NAME_TARGET);
            }
            else if (d.regType == D3DSPR_DEPTHOUT)
            {
                AppendSignatureEntry(out.outputs, "SV_Depth",
                                     FirstComponent(d.writeMask), 0,
                                     d.writeMask ? d.writeMask : 0x1,
                                     D3D_NAME_DEPTH);
                out.writesSVDepth = true;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Main per-instruction dispatch.
// ---------------------------------------------------------------------------
Bool TranslateOne(const SM3Instruction& sm3,
                  const std::vector<DefConst>& defs,
                  const std::unordered_map<Uint32, Int32>& defInts,
                  Uint32& numTemps,
                  Uint32  predBase,
                  std::vector<Uint32>& repStack,
                  D3DSW_ParsedShader& out)
{
    switch (sm3.op)
    {
    // ----- Declarations & pseudo-ops consumed at signature-construction time.
    case D3DSIO_DCL:
    case D3DSIO_DEF:
    case D3DSIO_DEFI:
    case D3DSIO_DEFB:
    case D3DSIO_NOP:
    case D3DSIO_COMMENT:
    case D3DSIO_PHASE:
    case D3DSIO_END:
        return true;

    // ----- 1:1 arithmetic ---------------------------------------------------
    case D3DSIO_MOV: return TranslateSimple(D3D10_SB_OPCODE_MOV, 1, sm3, defs, out);
    case D3DSIO_ADD: return TranslateSimple(D3D10_SB_OPCODE_ADD, 2, sm3, defs, out);
    case D3DSIO_MUL: return TranslateSimple(D3D10_SB_OPCODE_MUL, 2, sm3, defs, out);
    case D3DSIO_MAD: return TranslateSimple(D3D10_SB_OPCODE_MAD, 3, sm3, defs, out);
    case D3DSIO_DP3: return TranslateSimple(D3D10_SB_OPCODE_DP3, 2, sm3, defs, out);
    case D3DSIO_DP4: return TranslateSimple(D3D10_SB_OPCODE_DP4, 2, sm3, defs, out);
    case D3DSIO_MIN: return TranslateSimple(D3D10_SB_OPCODE_MIN, 2, sm3, defs, out);
    case D3DSIO_MAX: return TranslateSimple(D3D10_SB_OPCODE_MAX, 2, sm3, defs, out);
    case D3DSIO_FRC: return TranslateSimple(D3D10_SB_OPCODE_FRC, 1, sm3, defs, out);
    case D3DSIO_EXP: return TranslateSimple(D3D10_SB_OPCODE_EXP, 1, sm3, defs, out);
    case D3DSIO_LOG: return TranslateSimple(D3D10_SB_OPCODE_LOG, 1, sm3, defs, out);
    case D3DSIO_RSQ: return TranslateSimple(D3D10_SB_OPCODE_RSQ, 1, sm3, defs, out);
    case D3DSIO_RCP: return TranslateSimple(D3D11_SB_OPCODE_RCP, 1, sm3, defs, out);

    // ----- SUB dst, a, b  →  ADD dst, a, -b
    case D3DSIO_SUB:
    {
        if (sm3.operands.size() != 3) { return false; }
        SM3Instruction rewrite = sm3;
        rewrite.op = D3DSIO_ADD;
        rewrite.operands[2].srcMod =
            (rewrite.operands[2].srcMod == D3DSPSM_NEG)    ? D3DSPSM_NONE    :
            (rewrite.operands[2].srcMod == D3DSPSM_ABS)    ? D3DSPSM_ABSNEG  :
            (rewrite.operands[2].srcMod == D3DSPSM_ABSNEG) ? D3DSPSM_ABS     :
            (rewrite.operands[2].srcMod == D3DSPSM_NONE)   ? D3DSPSM_NEG     :
                                                             rewrite.operands[2].srcMod;
        return TranslateSimple(D3D10_SB_OPCODE_ADD, 2, rewrite, defs, out);
    }

    // ----- Matrix macros ----------------------------------------------------
    case D3DSIO_M4x4: return TranslateMatrixMul(D3D10_SB_OPCODE_DP4, 4, sm3, defs, out);
    case D3DSIO_M4x3: return TranslateMatrixMul(D3D10_SB_OPCODE_DP4, 3, sm3, defs, out);
    case D3DSIO_M3x4: return TranslateMatrixMul(D3D10_SB_OPCODE_DP3, 4, sm3, defs, out);
    case D3DSIO_M3x3: return TranslateMatrixMul(D3D10_SB_OPCODE_DP3, 3, sm3, defs, out);
    case D3DSIO_M3x2: return TranslateMatrixMul(D3D10_SB_OPCODE_DP3, 2, sm3, defs, out);

    // ----- Composite --------------------------------------------------------
    // ABS dst, src  →  MOV dst, |src|  (apply absolute modifier to source)
    case D3DSIO_ABS:
    {
        SM3Instruction rewrite = sm3;
        rewrite.op = D3DSIO_MOV;
        if (!rewrite.operands.empty() && rewrite.operands.size() > 1)
        {
            rewrite.operands[1].srcMod =
                (rewrite.operands[1].srcMod == D3DSPSM_NEG)    ? D3DSPSM_ABSNEG :
                (rewrite.operands[1].srcMod == D3DSPSM_ABSNEG) ? D3DSPSM_ABSNEG :
                                                                  D3DSPSM_ABS;
        }
        return TranslateSimple(D3D10_SB_OPCODE_MOV, 1, rewrite, defs, out);
    }
    case D3DSIO_LRP:    return TranslateLRP(sm3, defs, numTemps, out);
    case D3DSIO_SLT:    return TranslateSLT(sm3, defs, numTemps, out);
    case D3DSIO_SGE:    return TranslateSGE(sm3, defs, numTemps, out);
    case D3DSIO_CMP:    return TranslateCMP(sm3, defs, numTemps, out);
    case D3DSIO_NRM:    return TranslateNRM(sm3, defs, numTemps, out);
    case D3DSIO_POW:    return TranslatePOW(sm3, defs, numTemps, out);
    case D3DSIO_SINCOS: return TranslateSINCOS(sm3, defs, numTemps, out);
    case D3DSIO_DST:    return TranslateDST(sm3, defs, out);
    case D3DSIO_CRS:    return TranslateCRS(sm3, defs, numTemps, out);
    case D3DSIO_DP2ADD: return TranslateDP2ADD(sm3, defs, numTemps, out);
    case D3DSIO_SGN:    return TranslateSGN(sm3, defs, numTemps, out);
    case D3DSIO_LIT:    return TranslateLIT(sm3, defs, numTemps, out);
    case D3DSIO_TEXLDD: return TranslateTexldd(sm3, defs, out);

    // ----- Texture ----------------------------------------------------------
    case D3DSIO_TEX:
    {
        // TEXLD dst, src0, src1(sampler)
        // If D3DSI_TEXLD_PROJECT is set (controlBits bit 0): projective sample —
        // divide src0.xy by src0.w before sampling.
        if (sm3.controlBits & 0x01u)
        {
            // TEXLDP → emit: DIV r_tmp, src0, src0.wwww; SAMPLE dst, r_tmp, res, smp
            if (sm3.operands.size() != 3)                         { return false; }
            if (sm3.operands[2].regType != D3DSPR_SAMPLER)        { return false; }
            SM4Operand dst, uv, sampler;
            if (!ConvertDst(sm3.operands[0], dst))                { return false; }
            if (!ConvertSrc(sm3.operands[1], defs, uv))           { return false; }
            if (!ConvertSrc(sm3.operands[2], defs, sampler))      { return false; }

            Uint32 tmp = numTemps++;
            SM4Operand tmpDst{}, tmpSrc{};
            tmpDst.type = D3D10_SB_OPERAND_TYPE_TEMP; tmpDst.indexDim = 1;
            tmpDst.indices[0] = tmp; tmpDst.writeMask = 0xF;
            tmpSrc = tmpDst;
            tmpSrc.swizzle[0] = 0; tmpSrc.swizzle[1] = 1; tmpSrc.swizzle[2] = 2; tmpSrc.swizzle[3] = 3;

            // Build uv.wwww as divisor
            SM4Operand uvW = uv;
            uvW.swizzle[0] = uvW.swizzle[1] = uvW.swizzle[2] = uvW.swizzle[3] = uv.swizzle[3];

            SM4Instruction div = MakeInstr(D3D10_SB_OPCODE_DIV);
            div.operands = { tmpDst, uv, uvW };
            out.instrs.push_back(std::move(div));

            SM4Instruction samp = MakeInstr(D3D10_SB_OPCODE_SAMPLE);
            samp.saturate = (sm3.operands[0].dstMod == D3DSPDM_SATURATE);
            samp.operands = { dst, tmpSrc, ResourceOperand(sm3.operands[2].regNum), sampler };
            out.instrs.push_back(std::move(samp));
            return true;
        }
        if (sm3.controlBits & 0x02u)
        {
            return TranslateTexldb(sm3, defs, out);  // TEXLD with bias flag
        }
        return TranslateTexld(sm3, defs, out);        // plain TEXLD
    }
    case D3DSIO_TEXLDL: return TranslateTexldl(sm3, defs, out);  // explicit LOD (src0.w)

    // TEXKILL dst: discard pixel if any of dst.xyz < 0.
    // dst register is tested (read); no write occurs. PS only.
    case D3DSIO_TEXKILL:
    {
        if (sm3.operands.size() < 1) { return false; }
        SM4Operand src{};
        if (!ConvertSrc(sm3.operands[0], defs, src)) { return false; }

        Uint32 tmp = numTemps++;
        auto mkTmpDst = [&](Uint8 mask) {
            SM4Operand op{};
            op.type = D3D10_SB_OPERAND_TYPE_TEMP; op.indexDim = 1;
            op.indices[0] = tmp; op.writeMask = mask;
            return op;
        };
        auto mkTmpSrc = [&](Uint8 swz) {
            SM4Operand op{};
            op.type = D3D10_SB_OPERAND_TYPE_TEMP; op.indexDim = 1;
            op.indices[0] = tmp; op.writeMask = 0xF;
            op.swizzle[0] = op.swizzle[1] = op.swizzle[2] = op.swizzle[3] = swz;
            return op;
        };

        // LT tmp.x, src.x, 0   → tmp.x = (src.x < 0) ? 0xFFFFFFFF : 0
        SM4Operand zero = ImmF4(0.f, 0.f, 0.f, 0.f);
        SM4Operand srcX = src; srcX.swizzle[0] = srcX.swizzle[1] = srcX.swizzle[2] = srcX.swizzle[3] = src.swizzle[0];
        SM4Operand srcY = src; srcY.swizzle[0] = srcY.swizzle[1] = srcY.swizzle[2] = srcY.swizzle[3] = src.swizzle[1];
        SM4Operand srcZ = src; srcZ.swizzle[0] = srcZ.swizzle[1] = srcZ.swizzle[2] = srcZ.swizzle[3] = src.swizzle[2];

        { SM4Instruction lt = MakeInstr(D3D10_SB_OPCODE_LT); lt.operands = { mkTmpDst(0x1), srcX, zero }; out.instrs.push_back(lt); }
        { SM4Instruction lt = MakeInstr(D3D10_SB_OPCODE_LT); lt.operands = { mkTmpDst(0x2), srcY, zero }; out.instrs.push_back(lt); }
        { SM4Instruction lt = MakeInstr(D3D10_SB_OPCODE_LT); lt.operands = { mkTmpDst(0x4), srcZ, zero }; out.instrs.push_back(lt); }
        // OR together: any < 0
        { SM4Instruction or1 = MakeInstr(D3D10_SB_OPCODE_OR); or1.operands = { mkTmpDst(0x1), mkTmpSrc(0), mkTmpSrc(1) }; out.instrs.push_back(or1); }
        { SM4Instruction or2 = MakeInstr(D3D10_SB_OPCODE_OR); or2.operands = { mkTmpDst(0x1), mkTmpSrc(0), mkTmpSrc(2) }; out.instrs.push_back(or2); }
        // DISCARD if non-zero
        SM4Instruction disc = MakeInstr(D3D10_SB_OPCODE_DISCARD);
        disc.testNonZero = true;
        disc.operands = { mkTmpSrc(0) };
        out.instrs.push_back(std::move(disc));
        return true;
    }

    // ----- Address register -------------------------------------------------
    // MOVA dst, src  →  round src to int, store in temp (a0 substitute).
    // Relative CB access via a0 (c[a0.x+n]) is resolved as a separate pass;
    // for now we emit FTOI so the value is at least integer-typed in the temp.
    case D3DSIO_MOVA:
    {
        if (sm3.operands.size() != 2) { return false; }
        SM4Operand dst, src;
        if (!ConvertDst(sm3.operands[0], dst))     { return false; }
        if (!ConvertSrc(sm3.operands[1], defs, src)) { return false; }
        SM4Instruction ins = MakeInstr(D3D10_SB_OPCODE_FTOI);
        ins.operands = { dst, src };
        out.instrs.push_back(std::move(ins));
        return true;
    }

    // ----- Control flow (1:1) -----------------------------------------------
    case D3DSIO_IF:
    {
        if (sm3.operands.size() != 1) { return false; }
        SM4Instruction ins = MakeInstr(D3D10_SB_OPCODE_IF);
        SM4Operand src;
        if (!ConvertSrc(sm3.operands[0], defs, src)) { return false; }
        ins.operands.push_back(src);
        out.instrs.push_back(std::move(ins));
        return true;
    }
    case D3DSIO_ELSE:     out.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ELSE));    return true;
    case D3DSIO_ENDIF:    out.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ENDIF));   return true;
    case D3DSIO_BREAK:    out.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_BREAK));   return true;
    case D3DSIO_LOOP:     out.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_LOOP));    return true;
    case D3DSIO_ENDLOOP:  out.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ENDLOOP)); return true;
    case D3DSIO_RET:      out.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));     return true;

    // ----- REP/ENDREP -------------------------------------------------------
    // REP i# repeats the body i#.x times. Uses a float counter so the body
    // can emit ordinary ADD/MUL without integer operand support.
    // i#.x is looked up from DEFI declarations; unknown count defaults to 1.
    case D3DSIO_REP:
    {
        Uint32 count = 1;
        if (!sm3.operands.empty())
        {
            auto it = defInts.find(sm3.operands[0].regNum);
            if (it != defInts.end()) { count = static_cast<Uint32>(it->second); }
        }

        Uint32 ctr = numTemps++;
        repStack.push_back(ctr);

        // MOV ctr.x, float(count)
        SM4Operand ctrDst = TempDst(ctr); ctrDst.writeMask = 0x1;
        SM4Instruction imov = MakeInstr(D3D10_SB_OPCODE_MOV);
        imov.operands = { ctrDst, ImmF4(static_cast<Float>(count), 0.f, 0.f, 0.f) };
        out.instrs.push_back(std::move(imov));

        // LOOP
        out.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_LOOP));

        // BREAKC_Z ctr.x  (break when counter reaches 0)
        SM4Operand ctrSrcX = TempSrc(ctr);
        ctrSrcX.swizzle[0] = ctrSrcX.swizzle[1] = ctrSrcX.swizzle[2] = ctrSrcX.swizzle[3] = 0;
        SM4Instruction breakc = MakeInstr(D3D10_SB_OPCODE_BREAKC);
        breakc.testNonZero = false;  // BREAKC_Z: break if value == 0
        breakc.operands = { ctrSrcX };
        out.instrs.push_back(std::move(breakc));
        return true;
    }
    case D3DSIO_ENDREP:
    {
        if (repStack.empty()) { return false; }
        Uint32 ctr = repStack.back(); repStack.pop_back();

        // ADD ctr.x, ctr.x, -1.0  (decrement)
        SM4Operand ctrDst = TempDst(ctr); ctrDst.writeMask = 0x1;
        SM4Operand ctrSrcX = TempSrc(ctr);
        ctrSrcX.swizzle[0] = ctrSrcX.swizzle[1] = ctrSrcX.swizzle[2] = ctrSrcX.swizzle[3] = 0;
        SM4Instruction add = MakeInstr(D3D10_SB_OPCODE_ADD);
        add.operands = { ctrDst, ctrSrcX, ImmF4(-1.f, 0.f, 0.f, 0.f) };
        out.instrs.push_back(std::move(add));

        out.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ENDLOOP));
        return true;
    }

    // ----- Predicate / integer compare flow control ------------------------
    // SETP dst, src0, src1: compare src0 [op] src1, store bool in dst (predicate or temp).
    // controlBits bits 0-2 = D3DSHADER_COMPARISON (1=GT,2=EQ,3=GE,4=LT,5=NE,6=LE).
    case D3DSIO_SETP:
    {
        if (sm3.operands.size() != 3) { return false; }
        const Uint8 cmpFn = sm3.controlBits & 0x7u;
        D3D10_SB_OPCODE_TYPE sm4cmp;
        bool swapSrcs = false;
        switch (cmpFn)
        {
        case 1: sm4cmp = D3D10_SB_OPCODE_LT; swapSrcs = true; break;  // GT(a,b) = LT(b,a)
        case 2: sm4cmp = D3D10_SB_OPCODE_EQ; break;
        case 3: sm4cmp = D3D10_SB_OPCODE_GE; break;
        case 4: sm4cmp = D3D10_SB_OPCODE_LT; break;
        case 5: sm4cmp = D3D10_SB_OPCODE_NE; break;
        case 6: sm4cmp = D3D10_SB_OPCODE_GE; swapSrcs = true; break;  // LE(a,b) = GE(b,a)
        default: return false;
        }
        SM4Operand dst, s0, s1;
        if (!ConvertDst(sm3.operands[0], dst, predBase))          { return false; }
        if (!ConvertSrc(sm3.operands[1], defs, s0, predBase))     { return false; }
        if (!ConvertSrc(sm3.operands[2], defs, s1, predBase))     { return false; }
        SM4Instruction ins = MakeInstr(sm4cmp);
        ins.operands = swapSrcs ? std::vector<SM4Operand>{dst, s1, s0}
                                : std::vector<SM4Operand>{dst, s0, s1};
        out.instrs.push_back(std::move(ins));
        return true;
    }

    // IFC src0, src1: compare src0 [op] src1, open an if-block if true.
    // Uses same comparison encoding as SETP.
    case D3DSIO_IFC:
    {
        if (sm3.operands.size() != 2) { return false; }
        const Uint8 cmpFn = sm3.controlBits & 0x7u;
        D3D10_SB_OPCODE_TYPE sm4cmp;
        bool swapSrcs = false;
        switch (cmpFn)
        {
        case 1: sm4cmp = D3D10_SB_OPCODE_LT; swapSrcs = true; break;
        case 2: sm4cmp = D3D10_SB_OPCODE_EQ; break;
        case 3: sm4cmp = D3D10_SB_OPCODE_GE; break;
        case 4: sm4cmp = D3D10_SB_OPCODE_LT; break;
        case 5: sm4cmp = D3D10_SB_OPCODE_NE; break;
        case 6: sm4cmp = D3D10_SB_OPCODE_GE; swapSrcs = true; break;
        default: return false;
        }
        SM4Operand s0, s1;
        if (!ConvertSrc(sm3.operands[0], defs, s0, predBase)) { return false; }
        if (!ConvertSrc(sm3.operands[1], defs, s1, predBase)) { return false; }

        Uint32 tmp = numTemps++;
        SM4Operand tmpDst{}, tmpSrc{};
        tmpDst.type = D3D10_SB_OPERAND_TYPE_TEMP; tmpDst.indexDim = 1;
        tmpDst.indices[0] = tmp; tmpDst.writeMask = 0x1;
        tmpSrc = tmpDst; tmpSrc.writeMask = 0xF;
        tmpSrc.swizzle[0] = tmpSrc.swizzle[1] = tmpSrc.swizzle[2] = tmpSrc.swizzle[3] = 0;

        { SM4Instruction cmp = MakeInstr(sm4cmp);
          cmp.operands = swapSrcs ? std::vector<SM4Operand>{tmpDst, s1, s0}
                                  : std::vector<SM4Operand>{tmpDst, s0, s1};
          out.instrs.push_back(std::move(cmp)); }

        SM4Instruction ifIns = MakeInstr(D3D10_SB_OPCODE_IF);
        ifIns.testNonZero = true;
        ifIns.operands = { tmpSrc };
        out.instrs.push_back(std::move(ifIns));
        return true;
    }

    // BREAKC src: break if src is true (non-zero).
    // BREAKP src: same but src is a predicate register.
    case D3DSIO_BREAKC:
    case D3DSIO_BREAKP:
    {
        if (sm3.operands.size() < 1) { return false; }
        SM4Operand src{};
        if (!ConvertSrc(sm3.operands[0], defs, src, predBase)) { return false; }
        SM4Instruction ins = MakeInstr(D3D10_SB_OPCODE_BREAKC);
        ins.testNonZero = true;
        ins.operands = { src };
        out.instrs.push_back(std::move(ins));
        return true;
    }

    default:
        return false;
    }
}

}

// ---------------------------------------------------------------------------
// Public entry points.
// ---------------------------------------------------------------------------
Bool SM3Translate(const SM3DecodedShader& in, D3DSW_ParsedShader& out)
{
    out = {};
    out.type = in.isPixelShader ? D3DSW_ShaderType::Pixel : D3DSW_ShaderType::Vertex;
    out.csThreadGroupX = 1;
    out.csThreadGroupY = 1;
    out.csThreadGroupZ = 1;
    out.gsInstanceCount = 1;

    // Pass 1: collect DEFs (float and integer) and build signatures.
    std::vector<DefConst> defs;
    std::unordered_map<Uint32, Int32> defInts;  // i# register → first component
    for (const auto& ins : in.instructions)
    {
        if (ins.op == D3DSIO_DEF && !ins.operands.empty())
        {
            DefConst d{};
            d.regNum   = ins.operands[0].regNum;
            d.value[0] = ins.defFloat[0];
            d.value[1] = ins.defFloat[1];
            d.value[2] = ins.defFloat[2];
            d.value[3] = ins.defFloat[3];
            defs.push_back(d);
        }
        else if (ins.op == D3DSIO_DEFI && !ins.operands.empty())
        {
            // Store the first component (loop count) for REP translation.
            defInts[ins.operands[0].regNum] = ins.defInt[0];
        }
    }

    // CB slot 0 sizing: ignore DEF-only constants (they become IMMEDIATE32).
    // A CB is only bound when the shader reads c# values not covered by DEFs.
    Uint32 maxConstReg = 0;
    Bool   anyConstRef = false;
    for (const auto& ins : in.instructions)
    {
        if (ins.op == D3DSIO_DEF || ins.op == D3DSIO_DEFI || ins.op == D3DSIO_DEFB)
        {
            continue;
        }
        for (const auto& op : ins.operands)
        {
            if (op.regType != D3DSPR_CONST)              { continue; }
            if (op.kind   != SM3OperandKind::Source)     { continue; }
            if (FindDef(defs, op.regNum))                { continue; }
            if (op.regNum + 1 > maxConstReg) { maxConstReg = op.regNum + 1; }
            anyConstRef = true;
        }
    }

    BuildSignatures(in, out);

    // CB slot 0: sized to max referenced c# + 1. DEF'd constants never flow
    // through the CB (they become IMMEDIATE32 in ConvertSrc), so the CB only
    // needs to cover dynamic constants.
    if (anyConstRef)
    {
        D3DSW_CBufBinding cb{};
        cb.slot      = 0;
        cb.sizeVec4s = maxConstReg;
        out.cbufs.push_back(cb);
    }

    // Pass 2: translate instructions, tracking temps along the way.
    Uint32 numTemps = 0;
    for (const auto& ins : in.instructions)
    {
        for (const auto& op : ins.operands)
        {
            TrackTemps(op, numTemps);
        }
    }

    // Reserve a slot for the predicate register p0 if it appears anywhere.
    Uint32 predBase = numTemps;
    bool anyPred = false;
    for (const auto& ins : in.instructions)
    {
        for (const auto& op : ins.operands)
        {
            if (op.regType == D3DSPR_PREDICATE) { anyPred = true; break; }
        }
        if (anyPred) { break; }
    }
    if (anyPred) { numTemps++; }  // p0 → temp[predBase]

    // --- MISCTYPE lowering (vPos = reg 0, vFace = reg 1) ---
    // vPos  → SV_Position PS input (screen-space pixel center)
    // vFace → bool isFrontFace converted to +1.0 / -1.0 float via MOVC
    bool usesVPos = false, usesVFace = false;
    if (in.isPixelShader)
    {
        for (const auto& ins : in.instructions)
        {
            for (const auto& op : ins.operands)
            {
                if (op.regType == D3DSPR_MISCTYPE && op.kind == SM3OperandKind::Source)
                {
                    if (op.regNum == D3DSMO_POSITION) { usesVPos  = true; }
                    if (op.regNum == D3DSMO_FACE)     { usesVFace = true; }
                }
            }
        }
    }

    Uint32 vPosInputReg  = 0xFFu;
    Uint32 vFaceInputReg = 0xFFu;
    Uint32 vFaceTemp     = 0xFFu;

    if (usesVPos || usesVFace)
    {
        // Assign new input registers beyond those already DCL'd.
        Uint32 nextInReg = 0;
        for (const auto& e : out.inputs)
        {
            if (e.reg + 1u > nextInReg) { nextInReg = e.reg + 1u; }
        }

        if (usesVPos)
        {
            vPosInputReg = nextInReg++;
            AppendSignatureEntry(out.inputs, "SV_Position", 0, vPosInputReg, 0xF,
                                 D3D_NAME_POSITION);
        }
        if (usesVFace)
        {
            vFaceInputReg = nextInReg++;
            AppendSignatureEntry(out.inputs, "SV_IsFrontFace", 0, vFaceInputReg, 0x1,
                                 D3D_NAME_IS_FRONT_FACE);
            vFaceTemp = numTemps++;

            // First instruction: convert bool isFrontFace to +1.0 (front) / -1.0 (back).
            SM4Operand faceDst{};
            faceDst.type = D3D10_SB_OPERAND_TYPE_TEMP; faceDst.indexDim = 1;
            faceDst.indices[0] = vFaceTemp; faceDst.writeMask = 0xF;

            SM4Operand faceIn{};
            faceIn.type = D3D10_SB_OPERAND_TYPE_INPUT; faceIn.indexDim = 1;
            faceIn.indices[0] = vFaceInputReg;
            faceIn.swizzle[0] = faceIn.swizzle[1] = faceIn.swizzle[2] = faceIn.swizzle[3] = 0;

            SM4Instruction movc = MakeInstr(D3D10_SB_OPCODE_MOVC);
            movc.operands = { faceDst, faceIn,
                              ImmF4(1.f, 1.f, 1.f, 1.f), ImmF4(-1.f, -1.f, -1.f, -1.f) };
            out.instrs.push_back(std::move(movc));
        }
    }

    // Rewrite MISCTYPE source operands in a mutable copy of the instruction list.
    std::vector<SM3Instruction> instrs = in.instructions;
    if (usesVPos || usesVFace)
    {
        for (auto& ins : instrs)
        {
            for (auto& op : ins.operands)
            {
                if (op.regType != D3DSPR_MISCTYPE || op.kind != SM3OperandKind::Source)
                {
                    continue;
                }
                if (op.regNum == D3DSMO_POSITION && vPosInputReg != 0xFFu)
                {
                    op.regType = D3DSPR_INPUT;
                    op.regNum  = vPosInputReg;
                }
                else if (op.regNum == D3DSMO_FACE && vFaceTemp != 0xFFu)
                {
                    op.regType = D3DSPR_TEMP;
                    op.regNum  = vFaceTemp;
                }
            }
        }
    }

    std::vector<Uint32> repStack;
    //Collect subroutine bodies: instructions between LABEL n and its matching RET
    //Key = label register number, value = instruction list for that subroutine
    std::unordered_map<Uint32, std::vector<SM3Instruction>> subroutines;
    {
        Uint32 curLabel = 0xFFFFFFFFu;
        for (const auto& ins : instrs)
        {
            if (ins.op == D3DSIO_LABEL && !ins.operands.empty())
            {
                curLabel = ins.operands[0].regNum;
                subroutines[curLabel] = {};
            }
            else if (ins.op == D3DSIO_RET && curLabel != 0xFFFFFFFFu)
            {
                curLabel = 0xFFFFFFFFu;  
            }
            else if (curLabel != 0xFFFFFFFFu)
            {
                subroutines[curLabel].push_back(ins);
            }
        }
    }

    for (const auto& ins : instrs)
    {
        if (ins.op == D3DSIO_LABEL) 
        { 
            continue; 
        }

        if (ins.op == D3DSIO_RET)
        {
            out.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));
            break;
        }

        // CALL label_n: inline the subroutine body
        if (ins.op == D3DSIO_CALL)
        {
            if (ins.operands.empty()) 
            { 
                return false; 
            }
            Uint32 labelId = ins.operands[0].regNum;
            auto it = subroutines.find(labelId);
            if (it != subroutines.end())
            {
                for (const auto& subIns : it->second)
                {
                    if (!TranslateOne(subIns, defs, defInts, numTemps, predBase, repStack, out))
                        return false;
                }
            }
            continue;
        }

        // CALLNZ src, label_n: conditional inline wrapped in IF/ENDIF
        if (ins.op == D3DSIO_CALLNZ)
        {
            if (ins.operands.size() < 2) 
            { 
                return false; 
            }

            SM4Operand cond{};
            if (!ConvertSrc(ins.operands[0], defs, cond)) 
            { 
                return false; 
            }
            Uint32 labelId = ins.operands[1].regNum;
            SM4Instruction ifIns = MakeInstr(D3D10_SB_OPCODE_IF);
            ifIns.testNonZero = true;
            ifIns.operands = { cond };
            out.instrs.push_back(std::move(ifIns));
            auto it = subroutines.find(labelId);
            if (it != subroutines.end())
            {
                for (const auto& subIns : it->second)
                {
                    if (!TranslateOne(subIns, defs, defInts, numTemps, predBase, repStack, out))
                        return false;
                }
            }
            out.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ENDIF));
            continue;
        }

        if (!TranslateOne(ins, defs, defInts, numTemps, predBase, repStack, out))
        {
            return false;
        }
    }

    // Post-pass: record flags codegen + rasterizer consult.
    for (const auto& ins : out.instrs)
    {
        if (ins.op == D3D10_SB_OPCODE_DISCARD)
        {
            out.usesDiscard = true;
        }
        if (out.type == D3DSW_ShaderType::Pixel && ins.op == D3D10_SB_OPCODE_SAMPLE)
        {
            out.needsQuad = true;
        }
    }

    out.numTemps = numTemps;
    return true;
}

Bool SM3Parse(const void* bytecode, Usize len, D3DSW_ParsedShader& out)
{
    const DWORD* tokens   = static_cast<const DWORD*>(bytecode);
    const Usize  numTokens = len / sizeof(DWORD);
    SM3DecodedShader dec;
    if (!SM3Decode(tokens, numTokens, dec))
    {
        return false;
    }
    return SM3Translate(dec, out);
}

}
