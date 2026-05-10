#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <cstring>
#include <vector>

#include "d3d9/translate/sm3_decoder.h"
#include "d3d9/translate/sm3_ir.h"

using namespace d3dsw;

// ---- SM3 token builders ------------------------------------------------
namespace
{
    constexpr DWORD kTokEnd = 0x80000000u;  // parameter-token END marker bit

    DWORD EncRegType(DWORD type)
    {
        // Low 3 bits go to token bits 28-30; high 2 bits (type bits 3-4) go to
        // token bits 11-12. The d3d9types D3DSP_REGTYPE_SHIFT2=8 constant is
        // for decoding only (shifts bits 11-12 down to 3-4); for encoding the
        // matching reverse shift on the already-positioned bits 3-4 is +8.
        DWORD lo = (type & 0x7u)  << D3DSP_REGTYPE_SHIFT;   // bits 28-30
        DWORD hi = (type & 0x18u) << D3DSP_REGTYPE_SHIFT2;  // bits 3-4 → bits 11-12
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
        return kTokEnd
             | EncRegType(type)
             | (num & D3DSP_REGNUM_MASK)
             | ((writeMask & 0xFu) << 16)
             | dstMod;
    }

    DWORD Src(DWORD type, DWORD num, DWORD swizzle = D3DSP_NOSWIZZLE, DWORD srcMod = D3DSPSM_NONE)
    {
        return kTokEnd
             | EncRegType(type)
             | (num & D3DSP_REGNUM_MASK)
             | swizzle
             | srcMod;
    }

    DWORD FloatBits(Float f)
    {
        DWORD out = 0;
        std::memcpy(&out, &f, sizeof(Float));
        return out;
    }
}

// ---- Tests -------------------------------------------------------------

TEST(SM3Decoder, VersionHeader_VS30)
{
    const DWORD prog[] = { D3DVS_VERSION(3, 0), D3DPS_END() };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, 2, dec));
    EXPECT_EQ(dec.majorVersion, 3);
    EXPECT_EQ(dec.minorVersion, 0);
    EXPECT_FALSE(dec.isPixelShader);
    EXPECT_TRUE(dec.instructions.empty());
}

TEST(SM3Decoder, VersionHeader_PS30)
{
    const DWORD prog[] = { D3DPS_VERSION(3, 0), D3DPS_END() };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, 2, dec));
    EXPECT_EQ(dec.majorVersion, 3);
    EXPECT_TRUE(dec.isPixelShader);
}

TEST(SM3Decoder, VersionHeader_SM2_Accepted)
{
    const DWORD prog[] = { D3DVS_VERSION(2, 0), D3DVS_END() };
    SM3DecodedShader dec;
    EXPECT_TRUE(SM3Decode(prog, 2, dec));
    EXPECT_EQ(dec.majorVersion, 2u);
    EXPECT_FALSE(dec.isPixelShader);
}

TEST(SM3Decoder, VersionHeader_BadTag_Rejected)
{
    const DWORD prog[] = { 0xDEADBEEFu, D3DPS_END() };
    SM3DecodedShader dec;
    EXPECT_FALSE(SM3Decode(prog, 2, dec));
}

TEST(SM3Decoder, MOV_SingleDstSingleSrc)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_MOV, /*instLen=*/2),
        Dst(D3DSPR_TEMP,  0),
        Src(D3DSPR_CONST, 0),
        D3DVS_END()
    };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
    ASSERT_EQ(dec.instructions.size(), 1u);
    const auto& ins = dec.instructions[0];
    EXPECT_EQ(ins.op, D3DSIO_MOV);
    ASSERT_EQ(ins.operands.size(), 2u);

    EXPECT_EQ(ins.operands[0].kind,      SM3OperandKind::Destination);
    EXPECT_EQ(ins.operands[0].regType,   D3DSPR_TEMP);
    EXPECT_EQ(ins.operands[0].regNum,    0u);
    EXPECT_EQ(ins.operands[0].writeMask, 0xFu);

    EXPECT_EQ(ins.operands[1].kind,    SM3OperandKind::Source);
    EXPECT_EQ(ins.operands[1].regType, D3DSPR_CONST);
    EXPECT_EQ(ins.operands[1].regNum,  0u);
    EXPECT_EQ(ins.operands[1].swizzle[0], 0u);
    EXPECT_EQ(ins.operands[1].swizzle[1], 1u);
    EXPECT_EQ(ins.operands[1].swizzle[2], 2u);
    EXPECT_EQ(ins.operands[1].swizzle[3], 3u);
}

TEST(SM3Decoder, MAD_DstAndThreeSources)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_MAD, /*instLen=*/4),
        Dst(D3DSPR_TEMP,  0),
        Src(D3DSPR_INPUT, 0),
        Src(D3DSPR_CONST, 1),
        Src(D3DSPR_CONST, 2),
        D3DVS_END()
    };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
    ASSERT_EQ(dec.instructions.size(), 1u);
    const auto& ins = dec.instructions[0];
    EXPECT_EQ(ins.op, D3DSIO_MAD);
    ASSERT_EQ(ins.operands.size(), 4u);
    EXPECT_EQ(ins.operands[1].regType, D3DSPR_INPUT);
    EXPECT_EQ(ins.operands[2].regType, D3DSPR_CONST);
    EXPECT_EQ(ins.operands[3].regType, D3DSPR_CONST);
    EXPECT_EQ(ins.operands[3].regNum,  2u);
}

TEST(SM3Decoder, WriteMask_Decoding)
{
    // MOV r0.xz, c0
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_MOV, 2),
        Dst(D3DSPR_TEMP, 0, /*writeMask=*/0x5u),  // .x .z
        Src(D3DSPR_CONST, 0),
        D3DVS_END()
    };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
    EXPECT_EQ(dec.instructions[0].operands[0].writeMask, 0x5u);
}

TEST(SM3Decoder, Swizzle_XXYY)
{
    // MOV r0, c0.xxyy  → swizzle bits: x x y y = 00 00 01 01
    const DWORD swz = (0u << 16) | (0u << 18) | (1u << 20) | (1u << 22);
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_MOV, 2),
        Dst(D3DSPR_TEMP, 0),
        Src(D3DSPR_CONST, 0, swz),
        D3DVS_END()
    };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
    const auto& src = dec.instructions[0].operands[1];
    EXPECT_EQ(src.swizzle[0], 0u);
    EXPECT_EQ(src.swizzle[1], 0u);
    EXPECT_EQ(src.swizzle[2], 1u);
    EXPECT_EQ(src.swizzle[3], 1u);
}

TEST(SM3Decoder, SrcModifier_Negate)
{
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_MOV, 2),
        Dst(D3DSPR_TEMP, 0),
        Src(D3DSPR_CONST, 0, D3DSP_NOSWIZZLE, D3DSPSM_NEG),
        D3DVS_END()
    };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
    EXPECT_EQ(dec.instructions[0].operands[1].srcMod, D3DSPSM_NEG);
}

TEST(SM3Decoder, DCL_VertexInput)
{
    // dcl_texcoord1 v3
    const DWORD usage = (D3DDECLUSAGE_TEXCOORD << D3DSP_DCL_USAGE_SHIFT)
                      | (1u << D3DSP_DCL_USAGEINDEX_SHIFT)
                      | 0x80000000u;
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_DCL, 2),
        usage,
        Dst(D3DSPR_INPUT, 3),
        D3DVS_END()
    };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
    ASSERT_EQ(dec.instructions.size(), 1u);
    const auto& ins = dec.instructions[0];
    EXPECT_EQ(ins.op,             D3DSIO_DCL);
    EXPECT_EQ(ins.declUsage,      D3DDECLUSAGE_TEXCOORD);
    EXPECT_EQ(ins.declUsageIndex, 1u);
    ASSERT_EQ(ins.operands.size(), 1u);
    EXPECT_EQ(ins.operands[0].regType, D3DSPR_INPUT);
    EXPECT_EQ(ins.operands[0].regNum,  3u);
}

TEST(SM3Decoder, DCL_Sampler2D)
{
    // dcl_2d s0
    const DWORD usage = D3DSTT_2D | 0x80000000u;
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_DCL, 2),
        usage,
        Dst(D3DSPR_SAMPLER, 0),
        D3DPS_END()
    };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
    ASSERT_EQ(dec.instructions.size(), 1u);
    EXPECT_EQ(dec.instructions[0].declSamplerType, D3DSTT_2D);
    EXPECT_EQ(dec.instructions[0].operands[0].regType, D3DSPR_SAMPLER);
}

TEST(SM3Decoder, DEF_FloatImmediate)
{
    // def c1, 1.0, 2.0, 3.0, 4.0
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_DEF, 5),
        Dst(D3DSPR_CONST, 1),
        FloatBits(1.f), FloatBits(2.f), FloatBits(3.f), FloatBits(4.f),
        D3DVS_END()
    };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
    ASSERT_EQ(dec.instructions.size(), 1u);
    const auto& ins = dec.instructions[0];
    EXPECT_EQ(ins.op, D3DSIO_DEF);
    EXPECT_EQ(ins.operands[0].regType, D3DSPR_CONST);
    EXPECT_EQ(ins.operands[0].regNum,  1u);
    EXPECT_FLOAT_EQ(ins.defFloat[0], 1.f);
    EXPECT_FLOAT_EQ(ins.defFloat[1], 2.f);
    EXPECT_FLOAT_EQ(ins.defFloat[2], 3.f);
    EXPECT_FLOAT_EQ(ins.defFloat[3], 4.f);
}

TEST(SM3Decoder, IF_ELSE_ENDIF_NoDst)
{
    // setp... if_ne r0, r1 ... endif  — here we just test raw IF/ELSE/ENDIF (no dst, one or two srcs).
    const DWORD prog[] = {
        D3DPS_VERSION(3, 0),
        Opcode(D3DSIO_IF,    1), Src(D3DSPR_CONSTBOOL, 0),
        Opcode(D3DSIO_MOV,   2), Dst(D3DSPR_COLOROUT, 0), Src(D3DSPR_CONST, 0),
        Opcode(D3DSIO_ELSE,  0),
        Opcode(D3DSIO_MOV,   2), Dst(D3DSPR_COLOROUT, 0), Src(D3DSPR_CONST, 1),
        Opcode(D3DSIO_ENDIF, 0),
        D3DPS_END()
    };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
    ASSERT_EQ(dec.instructions.size(), 5u);
    EXPECT_EQ(dec.instructions[0].op, D3DSIO_IF);
    EXPECT_EQ(dec.instructions[0].operands.size(), 1u);  // just src, no dst
    EXPECT_EQ(dec.instructions[1].op, D3DSIO_MOV);
    EXPECT_EQ(dec.instructions[2].op, D3DSIO_ELSE);
    EXPECT_TRUE(dec.instructions[2].operands.empty());
    EXPECT_EQ(dec.instructions[3].op, D3DSIO_MOV);
    EXPECT_EQ(dec.instructions[4].op, D3DSIO_ENDIF);
    EXPECT_TRUE(dec.instructions[4].operands.empty());
}

TEST(SM3Decoder, CommentBlock_Skipped)
{
    const DWORD commentSize = 3;
    const DWORD commentTok  = D3DSIO_COMMENT | (commentSize << D3DSI_COMMENTSIZE_SHIFT);
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        commentTok, 0xAAAAAAAAu, 0xBBBBBBBBu, 0xCCCCCCCCu,  // 3-DWORD comment payload
        Opcode(D3DSIO_MOV, 2),
        Dst(D3DSPR_TEMP, 0),
        Src(D3DSPR_CONST, 0),
        D3DVS_END()
    };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
    ASSERT_EQ(dec.instructions.size(), 1u);
    EXPECT_EQ(dec.instructions[0].op, D3DSIO_MOV);
}

TEST(SM3Decoder, TruncatedInstruction_Rejected)
{
    // MOV claims instLen=2, but we only give it 1 operand before END of buffer.
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_MOV, 2),
        Dst(D3DSPR_TEMP, 0),
        // (missing src, and no END)
    };
    SM3DecodedShader dec;
    EXPECT_FALSE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
}

TEST(SM3Decoder, KitchenSink_VSWithDCLDEFMadEnd)
{
    const DWORD usagePos = (D3DDECLUSAGE_POSITION << D3DSP_DCL_USAGE_SHIFT) | 0x80000000u;
    const DWORD prog[] = {
        D3DVS_VERSION(3, 0),
        Opcode(D3DSIO_DCL, 2), usagePos, Dst(D3DSPR_INPUT, 0),
        Opcode(D3DSIO_DEF, 5), Dst(D3DSPR_CONST, 4),
            FloatBits(0.25f), FloatBits(0.5f), FloatBits(0.75f), FloatBits(1.f),
        Opcode(D3DSIO_MAD, 4),
            Dst(D3DSPR_TEMP, 0),
            Src(D3DSPR_INPUT, 0),
            Src(D3DSPR_CONST, 4),
            Src(D3DSPR_CONST, 4, D3DSP_NOSWIZZLE, D3DSPSM_NEG),
        D3DVS_END()
    };
    SM3DecodedShader dec;
    ASSERT_TRUE(SM3Decode(prog, sizeof(prog) / sizeof(DWORD), dec));
    ASSERT_EQ(dec.instructions.size(), 3u);
    EXPECT_EQ(dec.instructions[0].op, D3DSIO_DCL);
    EXPECT_EQ(dec.instructions[1].op, D3DSIO_DEF);
    EXPECT_FLOAT_EQ(dec.instructions[1].defFloat[2], 0.75f);
    EXPECT_EQ(dec.instructions[2].op, D3DSIO_MAD);
    EXPECT_EQ(dec.instructions[2].operands[3].srcMod, D3DSPSM_NEG);
}
