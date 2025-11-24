#include <gtest/gtest.h>
#include <d3d11TokenizedProgramFormat.hpp>
#include "shaders/dxbc_parser.h"
#include "shaders/sm4_decoder.h"
#include <vector>
#include <cstring>

using namespace d3d11sw;

static std::vector<Uint8> BuildDXBCBlob(Uint32 chunkFourCC, const std::vector<Uint8>& chunkPayload)
{
    // Layout:
    //   DXBCContainerHeader  (32 bytes)
    //   Uint32 offsets[1]    ( 4 bytes) — offset of our single chunk
    //   DXBCChunkHeader      ( 8 bytes) — at offset 36
    //   chunk payload

    const Uint32 chunkOffset  = 36;
    const Uint32 chunkSize    = static_cast<Uint32>(chunkPayload.size());
    const Uint32 totalSize    = chunkOffset + 8 + chunkSize;

    std::vector<Uint8> blob(totalSize, 0);
    Uint8* p = blob.data();

    Uint32 magic = MakeFourCC('D','X','B','C');
    std::memcpy(p,      &magic,      4); p += 4;
    p += 16;                              // checksum
    Uint32 version = 1;
    std::memcpy(p,      &version,    4); p += 4;
    std::memcpy(p,      &totalSize,  4); p += 4;
    Uint32 chunkCount = 1;
    std::memcpy(p,      &chunkCount, 4); p += 4;

    std::memcpy(p, &chunkOffset, 4); p += 4;

    std::memcpy(p, &chunkFourCC, 4); p += 4;
    std::memcpy(p, &chunkSize,   4); p += 4;

    std::memcpy(p, chunkPayload.data(), chunkSize);

    return blob;
}

static std::vector<Uint8> DwordsToBytes(const std::vector<Uint32>& dwords)
{
    std::vector<Uint8> bytes(dwords.size() * 4);
    std::memcpy(bytes.data(), dwords.data(), bytes.size());
    return bytes;
}

static std::vector<Uint32> MakeMinimalVSTokens()
{
    // Minimal VS SM4 token stream: dcl_temps 1 / mov r0.xyzw, v0.xyzw / ret
    return {
        0xFFFE0400,  // version: VS SM4.0
        0x0000000A,  // total = 10 DWORDs

        ENCODE_D3D10_SB_OPCODE_TYPE(D3D10_SB_OPCODE_DCL_TEMPS) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(2),
        0x00000001,  // numTemps = 1

        ENCODE_D3D10_SB_OPCODE_TYPE(D3D10_SB_OPCODE_MOV) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(5),
        0x001000F2,  // dst r0: Temp, mask xyzw, 1D index, imm32
        0x00000000,  // r0 index = 0
        0x00101E46,  // src v0: Input, swizzle xyzw, 1D index, imm32
        0x00000000,  // v0 index = 0

        ENCODE_D3D10_SB_OPCODE_TYPE(D3D10_SB_OPCODE_RET) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(1),
    };
}

static std::vector<Uint8> MakeISGNPayload()
{
    // ISGN payload with one element: "POSITION" at register 0, mask 0xF
    // 8-byte header + 24-byte element + name string
    const Uint32 elementCount = 1;
    const Uint32 unknown      = 8;
    const Uint32 nameOffset   = 8 + 24; // = 32

    std::vector<Uint8> data(32 + 9, 0); 
    Uint8* p = data.data();
    std::memcpy(p,      &elementCount, 4); p += 4;
    std::memcpy(p,      &unknown,      4); p += 4;

    // DXBCSigElement
    Uint32 semanticIndex  = 0;
    Uint32 svType         = 0;
    Uint32 componentType  = 3; 
    Uint32 registerIndex  = 0;
    Uint8  mask           = 0xF;
    Uint8  rwMask         = 0xF;

    std::memcpy(p,      &nameOffset,    4); p += 4;
    std::memcpy(p,      &semanticIndex, 4); p += 4;
    std::memcpy(p,      &svType,        4); p += 4;
    std::memcpy(p,      &componentType, 4); p += 4;
    std::memcpy(p,      &registerIndex, 4); p += 4;
    *p++ = mask;
    *p++ = rwMask;
    p += 2; 

    std::memcpy(p, "POSITION", 9);
    return data;
}


struct DXBCParserTests : ::testing::Test {};

TEST_F(DXBCParserTests, NullInputFails)
{
    DXBCParser parser;
    D3D11SW_ParsedShader out;
    EXPECT_FALSE(parser.ParseReflection(nullptr, 0, out));
}

TEST_F(DXBCParserTests, TooSmallFails)
{
    Uint8 tiny[4] = {0x44, 0x58, 0x42, 0x43}; // just "DXBC"
    DXBCParser parser;
    D3D11SW_ParsedShader out;
    EXPECT_FALSE(parser.ParseReflection(tiny, sizeof(tiny), out));
}

TEST_F(DXBCParserTests, WrongMagicFails)
{
    std::vector<Uint32> tokens = MakeMinimalVSTokens();
    auto blob = BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(tokens));
    blob[0] = 0xFF; // corrupt magic

    DXBCParser parser;
    D3D11SW_ParsedShader out;
    EXPECT_FALSE(parser.ParseReflection(blob.data(), blob.size(), out));
}

TEST_F(DXBCParserTests, ParseShdrChunkSucceeds)
{
    auto blob = BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(MakeMinimalVSTokens()));

    DXBCParser parser;
    D3D11SW_ParsedShader out;
    EXPECT_TRUE(parser.Parse(blob.data(), blob.size(), out));
}

TEST_F(DXBCParserTests, ParseShdrDecodesTemps)
{
    auto blob = BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(MakeMinimalVSTokens()));

    DXBCParser parser;
    D3D11SW_ParsedShader out;
    ASSERT_TRUE(parser.Parse(blob.data(), blob.size(), out));
    EXPECT_EQ(out.numTemps, 1u);
}

TEST_F(DXBCParserTests, ParseShdrInstructionCount)
{
    auto blob = BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(MakeMinimalVSTokens()));

    DXBCParser parser;
    D3D11SW_ParsedShader out;
    ASSERT_TRUE(parser.Parse(blob.data(), blob.size(), out));

    // dcl_temps + mov + ret = 3 instructions
    EXPECT_EQ(out.instrs.size(), 3u);
}

TEST_F(DXBCParserTests, ParseShdrMovOpcode)
{
    auto blob = BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(MakeMinimalVSTokens()));

    DXBCParser parser;
    D3D11SW_ParsedShader out;
    ASSERT_TRUE(parser.Parse(blob.data(), blob.size(), out));

    EXPECT_EQ(out.instrs[1].op, D3D10_SB_OPCODE_MOV);
}

TEST_F(DXBCParserTests, ParseShdrMovOperands)
{
    auto blob = BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(MakeMinimalVSTokens()));

    DXBCParser parser;
    D3D11SW_ParsedShader out;
    ASSERT_TRUE(parser.Parse(blob.data(), blob.size(), out));

    const SM4Instruction& mov = out.instrs[1];
    ASSERT_EQ(mov.operands.size(), 2u);

    // dst: r0, mask xyzw
    EXPECT_EQ(mov.operands[0].type, D3D10_SB_OPERAND_TYPE_TEMP);
    EXPECT_EQ(mov.operands[0].writeMask, 0xFu);
    EXPECT_EQ(mov.operands[0].indices[0], 0u);

    // src: v0, swizzle xyzw
    EXPECT_EQ(mov.operands[1].type, D3D10_SB_OPERAND_TYPE_INPUT);
    EXPECT_EQ(mov.operands[1].swizzle[0], 0u); // x
    EXPECT_EQ(mov.operands[1].swizzle[1], 1u); // y
    EXPECT_EQ(mov.operands[1].swizzle[2], 2u); // z
    EXPECT_EQ(mov.operands[1].swizzle[3], 3u); // w
    EXPECT_EQ(mov.operands[1].indices[0], 0u);
}

TEST_F(DXBCParserTests, ParseISGNOneElement)
{
    auto blob = BuildDXBCBlob(FOURCC_ISGN, MakeISGNPayload());

    DXBCParser parser;
    D3D11SW_ParsedShader out;
    ASSERT_TRUE(parser.ParseReflection(blob.data(), blob.size(), out));

    ASSERT_EQ(out.inputs.size(), 1u);
    EXPECT_STREQ(out.inputs[0].name, "POSITION");
    EXPECT_EQ(out.inputs[0].reg,           0u);
    EXPECT_EQ(out.inputs[0].mask,          0xFu);
    EXPECT_EQ(out.inputs[0].semanticIndex, 0u);
    EXPECT_EQ(out.inputs[0].svType,        0u);
}


struct SM4DecoderTests : ::testing::Test {};

TEST_F(SM4DecoderTests, EmptyStreamFails)
{
    SM4Decoder decoder;
    std::vector<SM4Instruction> out;
    Uint32 numTemps = 0;
    Uint32 tgs[3]   = {};
    EXPECT_FALSE(decoder.Decode(nullptr, 0, out, numTemps, tgs));
}

TEST_F(SM4DecoderTests, RetOnly)
{
    std::vector<Uint32> tokens = {
        0xFFFE0400,  // version
        0x00000003,  // total = 3
        0x0100003E,  // ret
    };

    SM4Decoder decoder;
    std::vector<SM4Instruction> out;
    Uint32 numTemps = 0;
    Uint32 tgs[3]   = {};
    EXPECT_TRUE(decoder.Decode(tokens.data(), static_cast<Uint32>(tokens.size()), out, numTemps, tgs));
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].op, D3D10_SB_OPCODE_RET);
}

TEST_F(SM4DecoderTests, DclTempsExtractsCount)
{
    auto tokens = MakeMinimalVSTokens();
    SM4Decoder decoder;
    std::vector<SM4Instruction> instrs;
    Uint32 numTemps = 0;
    Uint32 tgs[3]   = {};
    ASSERT_TRUE(decoder.Decode(tokens.data(), static_cast<Uint32>(tokens.size()), instrs, numTemps, tgs));
    EXPECT_EQ(numTemps, 1u);
}

TEST_F(SM4DecoderTests, DclThreadGroupExtractsSize)
{
    std::vector<Uint32> tokens = {
        0xFFFE0500,  // version CS SM5
        0x00000007,  // total = 7

        ENCODE_D3D10_SB_OPCODE_TYPE(D3D11_SB_OPCODE_DCL_THREAD_GROUP) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(4),
        0x00000008,  // X = 8
        0x00000001,  // Y = 1
        0x00000001,  // Z = 1

        ENCODE_D3D10_SB_OPCODE_TYPE(D3D10_SB_OPCODE_RET) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(1),
    };

    SM4Decoder decoder;
    std::vector<SM4Instruction> instrs;
    Uint32 numTemps = 0;
    Uint32 tgs[3]   = {};
    ASSERT_TRUE(decoder.Decode(tokens.data(), static_cast<Uint32>(tokens.size()), instrs, numTemps, tgs));
    EXPECT_EQ(tgs[0], 8u);
    EXPECT_EQ(tgs[1], 1u);
    EXPECT_EQ(tgs[2], 1u);
}
