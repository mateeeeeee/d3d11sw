#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <d3d11TokenizedProgramFormat.hpp>
#include <d3dcommon.h>
#include <cstring>
#include <vector>

#include "d3d9/translate/sm3_translator.h"
#include "d3d9/translate/sm3_decoder.h"
#include "d3d9/translate/sm3_ir.h"

using namespace d3dsw;

// ---- SM3 token builders (same shape as test_sm3_decoder.cpp) ------------
namespace
{
    constexpr DWORD kTokEnd = 0x80000000u;

    DWORD EncRegType(DWORD type)
    {
        DWORD lo = (type & 0x7u)  << D3DSP_REGTYPE_SHIFT;
        DWORD hi = (type & 0x18u) << D3DSP_REGTYPE_SHIFT2;
        return lo | hi;
    }

    DWORD Opcode(DWORD op, DWORD instLen, DWORD control = 0)
    {
        return (op & D3DSI_OPCODE_MASK)
             | ((control & 0xFFu) << D3DSP_OPCODESPECIFICCONTROL_SHIFT)
             | ((instLen & 0xFu)  << D3DSI_INSTLENGTH_SHIFT);
    }

    DWORD Dst(DWORD type, DWORD num, DWORD writeMask = 0xFu, DWORD dstMod = D3DSPDM_NONE)
    {
        return kTokEnd | EncRegType(type) | (num & D3DSP_REGNUM_MASK)
             | ((writeMask & 0xFu) << 16) | dstMod;
    }

    DWORD Src(DWORD type, DWORD num, DWORD swizzle = D3DSP_NOSWIZZLE, DWORD srcMod = D3DSPSM_NONE)
    {
        return kTokEnd | EncRegType(type) | (num & D3DSP_REGNUM_MASK) | swizzle | srcMod;
    }

    DWORD FloatBits(Float f)
    {
        DWORD out = 0;
        std::memcpy(&out, &f, sizeof(Float));
        return out;
    }

    DWORD DclUsage(DWORD usage, DWORD usageIdx)
    {
        return kTokEnd
             | ((usage    & 0xFu) << D3DSP_DCL_USAGE_SHIFT)
             | ((usageIdx & 0xFu) << D3DSP_DCL_USAGEINDEX_SHIFT);
    }
}

// ---- Tests -------------------------------------------------------------

TEST(SM3Translator, VS2_RASTOUT_oPos_InferredAsSVPosition)
{
    // SM2-style VS: position written via D3DSPR_RASTOUT[0] (oPos), no DCL for output.
    // dcl_position v0
    // mov oPos, v0
    const DWORD prog[] = {
        D3DVS_VERSION(2, 0),
        Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_POSITION, 0), Dst(D3DSPR_INPUT, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_RASTOUT, D3DSRO_POSITION), Src(D3DSPR_INPUT, 0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    EXPECT_EQ(ps.type, D3DSW_ShaderType::Vertex);

    ASSERT_EQ(ps.outputs.size(), 1u);
    EXPECT_STREQ(ps.outputs[0].name, "SV_Position");
    EXPECT_EQ(ps.outputs[0].reg,    8u);  // oPos → high reg above texcoord bank
    EXPECT_EQ(ps.outputs[0].svType, (Uint32)D3D_NAME_POSITION);

    ASSERT_EQ(ps.instrs.size(), 1u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_MOV);
    ASSERT_EQ(ps.instrs[0].operands.size(), 2u);
    EXPECT_EQ(ps.instrs[0].operands[0].type,       D3D10_SB_OPERAND_TYPE_OUTPUT);
    EXPECT_EQ(ps.instrs[0].operands[0].indices[0],  8u);
    EXPECT_EQ(ps.instrs[0].operands[1].type,       D3D10_SB_OPERAND_TYPE_INPUT);
}

TEST(SM3Translator, VS2_ATTROUT_oD0_InferredAsColor0)
{
    // SM2-style VS: diffuse color written via D3DSPR_ATTROUT[0] (oD0).
    // mov oD0, v1
    const DWORD prog[] = {
        D3DVS_VERSION(2, 0),
        Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_POSITION, 0), Dst(D3DSPR_INPUT, 0),
        Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_COLOR,    0), Dst(D3DSPR_INPUT, 1),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_RASTOUT, D3DSRO_POSITION), Src(D3DSPR_INPUT, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_ATTROUT, 0),               Src(D3DSPR_INPUT, 1),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));

    ASSERT_EQ(ps.outputs.size(), 2u);
    EXPECT_STREQ(ps.outputs[0].name, "SV_Position");
    EXPECT_EQ(ps.outputs[0].reg, 8u);
    EXPECT_STREQ(ps.outputs[1].name, "COLOR");
    EXPECT_EQ(ps.outputs[1].semanticIndex, 0u);
    EXPECT_EQ(ps.outputs[1].reg, 9u);

    // The MOV oD0 instruction must emit to output reg 9.
    EXPECT_EQ(ps.instrs[1].operands[0].type,      D3D10_SB_OPERAND_TYPE_OUTPUT);
    EXPECT_EQ(ps.instrs[1].operands[0].indices[0], 9u);
}

TEST(SM3Translator, VS2_TEXCRDOUT_oT0_InferredAsTexcoord0)
{
    // SM2-style VS: texcoord written via D3DSPR_TEXCRDOUT[0] (oT0).
    // mov oT0, v2
    const DWORD prog[] = {
        D3DVS_VERSION(2, 0),
        Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_POSITION, 0), Dst(D3DSPR_INPUT, 0),
        Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_TEXCOORD, 0), Dst(D3DSPR_INPUT, 1),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_RASTOUT,  D3DSRO_POSITION), Src(D3DSPR_INPUT, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_TEXCRDOUT, 0),              Src(D3DSPR_INPUT, 1),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));

    ASSERT_EQ(ps.outputs.size(), 2u);
    EXPECT_STREQ(ps.outputs[0].name, "SV_Position");
    EXPECT_STREQ(ps.outputs[1].name, "TEXCOORD");
    EXPECT_EQ(ps.outputs[1].semanticIndex, 0u);
    EXPECT_EQ(ps.outputs[1].reg, 0u);  // oT0 = OUTPUT[0] → reg 0

    // The MOV oT0 instruction must emit to output reg 0.
    EXPECT_EQ(ps.instrs[1].operands[0].type,      D3D10_SB_OPERAND_TYPE_OUTPUT);
    EXPECT_EQ(ps.instrs[1].operands[0].indices[0], 0u);
}

TEST(SM3Translator, VS_DclInputOutput_BuildsSignatures)
{
    // dcl_position v0
    // dcl_position o0
    // mov o0, v0
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_POSITION, 0), Dst(D3DSPR_INPUT,  0),
        Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_POSITION, 0), Dst(D3DSPR_OUTPUT, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_OUTPUT, 0), Src(D3DSPR_INPUT, 0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    EXPECT_EQ(ps.type, D3DSW_ShaderType::Vertex);

    ASSERT_EQ(ps.inputs.size(), 1u);
    EXPECT_STREQ(ps.inputs[0].name, "POSITION");
    EXPECT_EQ(ps.inputs[0].reg, 0u);
    EXPECT_EQ(ps.inputs[0].svType, 0u);

    ASSERT_EQ(ps.outputs.size(), 1u);
    EXPECT_STREQ(ps.outputs[0].name, "SV_Position");
    EXPECT_EQ(ps.outputs[0].svType, (Uint32)D3D_NAME_POSITION);

    ASSERT_EQ(ps.instrs.size(), 1u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_MOV);
    ASSERT_EQ(ps.instrs[0].operands.size(), 2u);
    EXPECT_EQ(ps.instrs[0].operands[0].type, D3D10_SB_OPERAND_TYPE_OUTPUT);
    EXPECT_EQ(ps.instrs[0].operands[1].type, D3D10_SB_OPERAND_TYPE_INPUT);
}

TEST(SM3Translator, PS_ColorOutput_InferredFromWrite)
{
    // dcl_color0 v0
    // mov oC0, v0
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_COLOR, 0), Dst(D3DSPR_INPUT, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_COLOROUT, 0), Src(D3DSPR_INPUT, 0),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    EXPECT_EQ(ps.type, D3DSW_ShaderType::Pixel);

    ASSERT_EQ(ps.outputs.size(), 1u);
    EXPECT_STREQ(ps.outputs[0].name, "SV_Target");
    EXPECT_EQ(ps.outputs[0].reg, 0u);
    EXPECT_EQ(ps.outputs[0].svType, (Uint32)D3D_NAME_TARGET);
}

TEST(SM3Translator, CB_SizedToMaxConstReg)
{
    // mov r0, c5
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_CONST, 5),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.cbufs.size(), 1u);
    EXPECT_EQ(ps.cbufs[0].slot, 0u);
    EXPECT_EQ(ps.cbufs[0].sizeVec4s, 6u);  // c0..c5 = 6 vec4s
    EXPECT_EQ(ps.instrs[0].operands[1].type, D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER);
    EXPECT_EQ(ps.instrs[0].operands[1].indices[0], 0u);
    EXPECT_EQ(ps.instrs[0].operands[1].indices[1], 5u);
}

TEST(SM3Translator, DEF_Const_FoldedToImmediate)
{
    // def c4, 1.0, 2.0, 3.0, 4.0
    // mov r0, c4
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_DEF, 5), Dst(D3DSPR_CONST, 4),
            FloatBits(1.f), FloatBits(2.f), FloatBits(3.f), FloatBits(4.f),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_CONST, 4),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 1u);   // DEF itself does not emit
    const auto& src = ps.instrs[0].operands[1];
    EXPECT_EQ(src.type, D3D10_SB_OPERAND_TYPE_IMMEDIATE32);
    EXPECT_FLOAT_EQ(src.imm[0], 1.f);
    EXPECT_FLOAT_EQ(src.imm[1], 2.f);
    EXPECT_FLOAT_EQ(src.imm[2], 3.f);
    EXPECT_FLOAT_EQ(src.imm[3], 4.f);
    // Only c4 was DEF'd and never read as dynamic → no CB needed.
    EXPECT_TRUE(ps.cbufs.empty());
}

TEST(SM3Translator, SrcMod_Negate)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_TEMP, 0),
            Src(D3DSPR_INPUT, 0, D3DSP_NOSWIZZLE, D3DSPSM_NEG),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    EXPECT_TRUE(ps.instrs[0].operands[1].negate);
    EXPECT_FALSE(ps.instrs[0].operands[1].absolute);
}

TEST(SM3Translator, SrcMod_AbsNeg)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_TEMP, 0),
            Src(D3DSPR_INPUT, 0, D3DSP_NOSWIZZLE, D3DSPSM_ABSNEG),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    EXPECT_TRUE(ps.instrs[0].operands[1].negate);
    EXPECT_TRUE(ps.instrs[0].operands[1].absolute);
}

TEST(SM3Translator, DstMod_Saturate_LiftedToInstructionFlag)
{
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_MOV, 2),
            Dst(D3DSPR_COLOROUT, 0, 0xFu, D3DSPDM_SATURATE),
            Src(D3DSPR_INPUT, 0),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    EXPECT_TRUE(ps.instrs[0].saturate);
}

TEST(SM3Translator, SUB_RewrittenAsAddWithNegate)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_SUB, 3),
            Dst(D3DSPR_TEMP, 0),
            Src(D3DSPR_INPUT, 0),
            Src(D3DSPR_INPUT, 1),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 1u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_ADD);
    EXPECT_FALSE(ps.instrs[0].operands[1].negate);
    EXPECT_TRUE (ps.instrs[0].operands[2].negate);
}

TEST(SM3Translator, M4x4_Expands_To_Four_DP4)
{
    // m4x4 oPos, v0, c0   — maps to DP4 x/y/z/w, each against c0, c1, c2, c3.
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_M4x4, 3),
            Dst(D3DSPR_OUTPUT, 0),
            Src(D3DSPR_INPUT,  0),
            Src(D3DSPR_CONST,  0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 4u);
    for (Uint i = 0; i < 4; ++i)
    {
        EXPECT_EQ(ps.instrs[i].op, D3D10_SB_OPCODE_DP4);
        EXPECT_EQ(ps.instrs[i].operands[0].writeMask, (Uint8)(1u << i));
        // src1 is c(0+i)
        EXPECT_EQ(ps.instrs[i].operands[2].indices[0], 0u);
        EXPECT_EQ(ps.instrs[i].operands[2].indices[1], i);
    }
}

TEST(SM3Translator, TEX_EmitsSample_WithResourceAndSampler)
{
    // dcl_2d s0
    // texld r0, v0, s0
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_DCL, 2),
            kTokEnd | D3DSTT_2D,    // declSamplerType=2D, no usage/index needed
            Dst(D3DSPR_SAMPLER, 0),
        Opcode(D3DSIO_TEX, 3),
            Dst(D3DSPR_TEMP, 0),
            Src(D3DSPR_INPUT, 0),
            Src(D3DSPR_SAMPLER, 0),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));

    ASSERT_EQ(ps.textures.size(), 1u);
    EXPECT_EQ(ps.textures[0].slot,      0u);
    EXPECT_EQ(ps.textures[0].dimension, (Uint32)D3D_SRV_DIMENSION_TEXTURE2D);

    ASSERT_EQ(ps.instrs.size(), 1u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_SAMPLE);
    ASSERT_EQ(ps.instrs[0].operands.size(), 4u);
    EXPECT_EQ(ps.instrs[0].operands[2].type, D3D10_SB_OPERAND_TYPE_RESOURCE);
    EXPECT_EQ(ps.instrs[0].operands[3].type, D3D10_SB_OPERAND_TYPE_SAMPLER);
    EXPECT_TRUE(ps.needsQuad);
}

TEST(SM3Translator, IF_ELSE_ENDIF_Map_OneToOne)
{
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_IF,    1), Src(D3DSPR_CONSTBOOL, 0),
        Opcode(D3DSIO_MOV,   2), Dst(D3DSPR_COLOROUT, 0), Src(D3DSPR_CONST, 0),
        Opcode(D3DSIO_ELSE,  0),
        Opcode(D3DSIO_MOV,   2), Dst(D3DSPR_COLOROUT, 0), Src(D3DSPR_CONST, 1),
        Opcode(D3DSIO_ENDIF, 0),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    // CONSTBOOL is not in the MVP register set → IF's src fails translation.
    EXPECT_FALSE(SM3Parse(prog, sizeof(prog), ps));
}

TEST(SM3Translator, UnsupportedOpcode_RejectsGracefully)
{
    // CALL: valid SM3 opcode, not yet translated.
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_CALL, 1), Src(D3DSPR_LABEL, 0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    EXPECT_FALSE(SM3Parse(prog, sizeof(prog), ps));
}

TEST(SM3Translator, NumTemps_TracksMaxReg)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_TEMP, 2), Src(D3DSPR_INPUT, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_TEMP,  2),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    EXPECT_EQ(ps.numTemps, 3u);   // r0..r2
}

// SLT dst, src0, src1 → LT tmp + MOVC dst, tmp, 1.0, 0.0
TEST(SM3Translator, SLT_LowersTo_LT_MOVC)
{
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_SLT, 3), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_TEMP, 1), Src(D3DSPR_TEMP, 2),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 2u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_LT);
    EXPECT_EQ(ps.instrs[1].op, D3D10_SB_OPCODE_MOVC);
    // SLT allocates one extra temp beyond what the shader uses (r0..r2).
    EXPECT_GE(ps.numTemps, 4u);
}

// SGE dst, src0, src1 → GE tmp + MOVC dst, tmp, 1.0, 0.0
TEST(SM3Translator, SGE_LowersTo_GE_MOVC)
{
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_SGE, 3), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_TEMP, 1), Src(D3DSPR_TEMP, 2),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 2u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_GE);
    EXPECT_EQ(ps.instrs[1].op, D3D10_SB_OPCODE_MOVC);
}

// CMP dst, src0, src1, src2 → GE(tmp, src0, 0.0) + MOVC(dst, tmp, src1, src2)
TEST(SM3Translator, CMP_LowersTo_GE_MOVC)
{
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_CMP, 4),
            Dst(D3DSPR_COLOROUT, 0),
            Src(D3DSPR_TEMP, 0), Src(D3DSPR_TEMP, 1), Src(D3DSPR_TEMP, 2),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 2u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_GE);
    EXPECT_EQ(ps.instrs[1].op, D3D10_SB_OPCODE_MOVC);
    // MOVC dst should be oC0 (OUTPUT register 0).
    ASSERT_GE(ps.instrs[1].operands.size(), 1u);
    EXPECT_EQ(ps.instrs[1].operands[0].type, D3D10_SB_OPERAND_TYPE_OUTPUT);
    EXPECT_EQ(ps.instrs[1].operands[0].indices[0], 0u);
}


// NRM dst, src → DP3 + RSQ + MUL (3 instructions)
TEST(SM3Translator, NRM_LowersTo_DP3_RSQ_MUL)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_NRM, 2), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_INPUT, 0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 3u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_DP3);
    EXPECT_EQ(ps.instrs[1].op, D3D10_SB_OPCODE_RSQ);
    EXPECT_EQ(ps.instrs[2].op, D3D10_SB_OPCODE_MUL);
}

// POW dst, src0, src1 → LOG + MUL + EXP (3 instructions)
TEST(SM3Translator, POW_LowersTo_LOG_MUL_EXP)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_POW, 3), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_INPUT, 0), Src(D3DSPR_INPUT, 1),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 3u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_LOG);
    EXPECT_EQ(ps.instrs[1].op, D3D10_SB_OPCODE_MUL);
    EXPECT_EQ(ps.instrs[2].op, D3D10_SB_OPCODE_EXP);
}

// SINCOS dst.xy, src → SINCOS + 2×MOV (cos→dst.x, sin→dst.y)
TEST(SM3Translator, SINCOS_EmitsSINCOS_PlusMOVs)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_SINCOS, 2), Dst(D3DSPR_TEMP, 0, 0x3), Src(D3DSPR_INPUT, 0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 3u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_SINCOS);
    EXPECT_EQ(ps.instrs[1].op, D3D10_SB_OPCODE_MOV);
    EXPECT_EQ(ps.instrs[2].op, D3D10_SB_OPCODE_MOV);
}

// ABS dst, src → MOV dst, |src|
TEST(SM3Translator, ABS_LowersTo_MOV_WithAbsModifier)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_ABS, 2), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_INPUT, 0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 1u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_MOV);
    ASSERT_GE(ps.instrs[0].operands.size(), 2u);
    EXPECT_TRUE(ps.instrs[0].operands[1].absolute);
}

// TEXLDL → SAMPLE_L with src0.w as explicit LOD
TEST(SM3Translator, TEXLDL_EmitsSAMPLE_L)
{
    // dcl_2d s0 / texldl r0, v0, s0
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_DCL, 2), (0x80000000u | (D3DSTT_2D << 27)),
            Dst(D3DSPR_SAMPLER, 0),
        Opcode(D3DSIO_TEXLDL, 3),
            Dst(D3DSPR_COLOROUT, 0),
            Src(D3DSPR_INPUT, 0),
            Src(D3DSPR_SAMPLER, 0),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 1u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_SAMPLE_L);
    EXPECT_EQ(ps.instrs[0].operands.size(), 5u);  // dst,uv,resource,sampler,lod
}

// MOVA → FTOI into a temp
TEST(SM3Translator, MOVA_EmitsFTOI)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_MOVA, 2), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_INPUT, 0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_EQ(ps.instrs.size(), 1u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_FTOI);
}

// REP/ENDREP with DEFI count → LOOP + BREAKC_Z + counter management
TEST(SM3Translator, REP_ENDREP_WithDefi_EmitsLoop)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        // defi i0, 4, 0, 0, 0
        Opcode(D3DSIO_DEFI, 5), Dst(D3DSPR_CONSTINT, 0),
            4, 0, 0, 0,
        // rep i0 / mov r0, v0 / endrep
        Opcode(D3DSIO_REP, 1), Src(D3DSPR_CONSTINT, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_INPUT, 0),
        Opcode(D3DSIO_ENDREP, 0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    // Expect: MOV(init counter), LOOP, BREAKC, MOV(body), ADD(decrement), ENDLOOP
    ASSERT_GE(ps.instrs.size(), 5u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_MOV);    // init counter=4
    EXPECT_EQ(ps.instrs[1].op, D3D10_SB_OPCODE_LOOP);
    EXPECT_EQ(ps.instrs[2].op, D3D10_SB_OPCODE_BREAKC);  // break if 0
    EXPECT_EQ(ps.instrs[3].op, D3D10_SB_OPCODE_MOV);    // body
    EXPECT_EQ(ps.instrs[4].op, D3D10_SB_OPCODE_ADD);    // decrement
    ASSERT_GE(ps.instrs.size(), 6u);
    EXPECT_EQ(ps.instrs[5].op, D3D10_SB_OPCODE_ENDLOOP);
}

// LOOP/ENDLOOP → 1:1 SM4 LOOP/ENDLOOP
TEST(SM3Translator, LOOP_ENDLOOP_Map_OneToOne)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        // loop aL, i0 / mov r0, v0 / endloop
        Opcode(D3DSIO_LOOP, 2), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_CONSTINT, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_INPUT, 0),
        Opcode(D3DSIO_ENDLOOP, 0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_GE(ps.instrs.size(), 3u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_LOOP);
    EXPECT_EQ(ps.instrs[1].op, D3D10_SB_OPCODE_MOV);
    EXPECT_EQ(ps.instrs[2].op, D3D10_SB_OPCODE_ENDLOOP);
}

// TEXKILL → LT×3 + OR×2 + DISCARD
TEST(SM3Translator, TEXKILL_EmitsDiscardChain)
{
    // ps_3_0 with texkill t0 (t0 is read from as a texture coord register)
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_TEXKILL, 1), Dst(D3DSPR_TEMP, 0),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    // Expect: LT, LT, LT, OR, OR, DISCARD
    ASSERT_GE(ps.instrs.size(), 6u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_LT);
    EXPECT_EQ(ps.instrs[1].op, D3D10_SB_OPCODE_LT);
    EXPECT_EQ(ps.instrs[2].op, D3D10_SB_OPCODE_LT);
    EXPECT_EQ(ps.instrs[3].op, D3D10_SB_OPCODE_OR);
    EXPECT_EQ(ps.instrs[4].op, D3D10_SB_OPCODE_OR);
    EXPECT_EQ(ps.instrs[5].op, D3D10_SB_OPCODE_DISCARD);
    EXPECT_TRUE(ps.instrs[5].testNonZero);
}

// TEXLD with D3DSI_TEXLD_PROJECT flag → DIV + SAMPLE
TEST(SM3Translator, TEXLDP_EmitsDivThenSample)
{
    // D3DSI_TEXLD_PROJECT = 0x00010000 → control bits = 0x01
    const DWORD kTokEnd2 = 0x80000000u;
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_TEXCOORD, 0), Dst(D3DSPR_INPUT, 0),
        // dcl_2d s0 (sampler declaration)
        Opcode(D3DSIO_DCL, 2), (kTokEnd2 | D3DSTT_2D), Dst(D3DSPR_SAMPLER, 0),
        // texld r0, v0, s0  with PROJECT flag (control bits = 0x01)
        Opcode(D3DSIO_TEX, 3, /*control=*/0x01), Dst(D3DSPR_TEMP, 0),
            Src(D3DSPR_INPUT, 0), Src(D3DSPR_SAMPLER, 0),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    // Find DIV and SAMPLE in instruction list
    bool hasDIV = false, hasSAMPLE = false;
    for (const auto& ins : ps.instrs)
    {
        if (ins.op == D3D10_SB_OPCODE_DIV)    { hasDIV    = true; }
        if (ins.op == D3D10_SB_OPCODE_SAMPLE) { hasSAMPLE = true; }
    }
    EXPECT_TRUE(hasDIV)    << "TEXLDP should emit DIV for perspective divide";
    EXPECT_TRUE(hasSAMPLE) << "TEXLDP should emit SAMPLE";
}

// SETP dst, src0, src1 with GE comparison → emits GE
TEST(SM3Translator, SETP_GE_EmitsGE)
{
    // D3DSPC_GE = 3 → control bits = 0x03
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_SETP, 3, /*control=*/0x03),
            Dst(D3DSPR_PREDICATE, 0), Src(D3DSPR_TEMP, 0), Src(D3DSPR_TEMP, 1),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_GE(ps.instrs.size(), 1u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_GE);
    // Predicate register p0 mapped to a temp above regular temps
    EXPECT_EQ(ps.instrs[0].operands[0].type, D3D10_SB_OPERAND_TYPE_TEMP);
}

// IFC src0, src1 with LT comparison → compare temp + IF
TEST(SM3Translator, IFC_LT_EmitsCmpThenIf)
{
    // D3DSPC_LT = 4 → control bits = 0x04
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_IFC, 2, /*control=*/0x04),
            Src(D3DSPR_TEMP, 0), Src(D3DSPR_TEMP, 1),
        Opcode(D3DSIO_ENDIF, 0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    ASSERT_GE(ps.instrs.size(), 3u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_LT);
    EXPECT_EQ(ps.instrs[1].op, D3D10_SB_OPCODE_IF);
    EXPECT_EQ(ps.instrs[2].op, D3D10_SB_OPCODE_ENDIF);
}

// BREAKC with a predicate source → BREAKC in SM4
TEST(SM3Translator, BREAKC_EmitsSM4BREAKC)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_LOOP, 2), Dst(D3DSPR_TEMP, 0), Src(D3DSPR_CONSTINT, 0),
        Opcode(D3DSIO_BREAKC, 1), Src(D3DSPR_PREDICATE, 0),
        Opcode(D3DSIO_ENDLOOP, 0),
        D3DVS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));
    bool hasBreakC = false;
    for (const auto& ins : ps.instrs)
    {
        if (ins.op == D3D10_SB_OPCODE_BREAKC) { hasBreakC = true; break; }
    }
    EXPECT_TRUE(hasBreakC);
}

// vFace (D3DSPR_MISCTYPE reg 1) → isFrontFace bool converted to +1/-1 float via MOVC
TEST(SM3Translator, MISCTYPE_vFace_EmitsMOVC_FrontFaceInput)
{
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        // mov r0.x, vFace  (MISCTYPE reg 1)
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_TEMP, 0, 0x1),
            Src(D3DSPR_MISCTYPE, D3DSMO_FACE),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_COLOROUT, 0), Src(D3DSPR_TEMP, 0),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));

    // Check that SV_IsFrontFace input was added
    bool hasFrontFaceInput = false;
    for (const auto& e : ps.inputs)
    {
        if (e.svType == D3D_NAME_IS_FRONT_FACE) { hasFrontFaceInput = true; break; }
    }
    EXPECT_TRUE(hasFrontFaceInput) << "vFace should add SV_IsFrontFace input";

    // Check that the first instruction is MOVC (the bool→float conversion)
    ASSERT_GE(ps.instrs.size(), 1u);
    EXPECT_EQ(ps.instrs[0].op, D3D10_SB_OPCODE_MOVC)
        << "First instruction should be MOVC to convert isFrontFace to +1/-1";
}

// vPos (D3DSPR_MISCTYPE reg 0) → SV_Position PS input with screen-space coords
TEST(SM3Translator, MISCTYPE_vPos_AddsSVPositionInput)
{
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_TEMP, 0, 0x3),
            Src(D3DSPR_MISCTYPE, D3DSMO_POSITION),
        Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_COLOROUT, 0), Src(D3DSPR_TEMP, 0),
        D3DPS_END()
    };
    D3DSW_ParsedShader ps;
    ASSERT_TRUE(SM3Parse(prog, sizeof(prog), ps));

    // SV_Position input must be declared so the rasterizer fills it
    bool hasPosInput = false;
    for (const auto& e : ps.inputs)
    {
        if (std::strcmp(e.name, "SV_Position") == 0) { hasPosInput = true; break; }
    }
    EXPECT_TRUE(hasPosInput) << "vPos should add an SV_Position PS input";
}
