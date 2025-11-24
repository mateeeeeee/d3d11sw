#include <gtest/gtest.h>
#include "shaders/shader_jit.h"
#include "shaders/shader_abi.h"
#include "shaders/dxbc.h"
#include <d3d11TokenizedProgramFormat.hpp>
#include <vector>
#include <cstring>

using namespace d3d11sw;

static std::vector<Uint8> BuildDXBCBlob(Uint32 chunkFourCC, const std::vector<Uint8>& payload)
{
    const Uint32 chunkOffset = 36;
    const Uint32 chunkSize   = static_cast<Uint32>(payload.size());
    const Uint32 totalSize   = chunkOffset + 8 + chunkSize;

    std::vector<Uint8> blob(totalSize, 0);
    Uint8* p = blob.data();

    Uint32 magic      = MakeFourCC('D','X','B','C');
    Uint32 version    = 1;
    Uint32 chunkCount = 1;
    std::memcpy(p,      &magic,       4); p += 4;
    p += 16;
    std::memcpy(p,      &version,     4); p += 4;
    std::memcpy(p,      &totalSize,   4); p += 4;
    std::memcpy(p,      &chunkCount,  4); p += 4;
    std::memcpy(p,      &chunkOffset, 4); p += 4;
    std::memcpy(p,      &chunkFourCC, 4); p += 4;
    std::memcpy(p,      &chunkSize,   4); p += 4;
    std::memcpy(p, payload.data(), chunkSize);
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
    return {
        0xFFFE0400,
        0x00000004,
        ENCODE_D3D10_SB_OPCODE_TYPE(D3D10_SB_OPCODE_DCL_TEMPS) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(2),
        0x00000001,
        ENCODE_D3D10_SB_OPCODE_TYPE(D3D10_SB_OPCODE_RET) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(1),
    };
}

static std::vector<Uint32> MakeMinimalVSTokens2Temps()
{
    return {
        0xFFFE0400,
        0x00000004,
        ENCODE_D3D10_SB_OPCODE_TYPE(D3D10_SB_OPCODE_DCL_TEMPS) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(2),
        0x00000002,  // numTemps = 2 — different bytecode
        ENCODE_D3D10_SB_OPCODE_TYPE(D3D10_SB_OPCODE_RET) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(1),
    };
}

struct ShaderJITTests : ::testing::Test
{
    ShaderJIT jit;
};

TEST_F(ShaderJITTests, NullBytecodeReturnsNull)
{
    EXPECT_EQ(jit.GetOrCompile(nullptr, 0, D3D11SW_ShaderType::Vertex), nullptr);
}

TEST_F(ShaderJITTests, InvalidMagicReturnsNull)
{
    Uint8 garbage[64];
    EXPECT_EQ(jit.GetOrCompile(garbage, sizeof(garbage), D3D11SW_ShaderType::Vertex), nullptr);
}

TEST_F(ShaderJITTests, ValidVSReturnsNonNull)
{
    auto blob = BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(MakeMinimalVSTokens()));
    void* fn = jit.GetOrCompile(blob.data(), blob.size(), D3D11SW_ShaderType::Vertex);
    EXPECT_NE(fn, nullptr);
}

TEST_F(ShaderJITTests, CacheHitReturnsSamePointer)
{
    auto blob = BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(MakeMinimalVSTokens()));
    void* fn1 = jit.GetOrCompile(blob.data(), blob.size(), D3D11SW_ShaderType::Vertex);
    void* fn2 = jit.GetOrCompile(blob.data(), blob.size(), D3D11SW_ShaderType::Vertex);
    ASSERT_NE(fn1, nullptr);
    EXPECT_EQ(fn1, fn2);
}

TEST_F(ShaderJITTests, DifferentBytecodeDifferentPointer)
{
    auto blob1 = BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(MakeMinimalVSTokens()));
    auto blob2 = BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(MakeMinimalVSTokens2Temps()));
    void* fn1 = jit.GetOrCompile(blob1.data(), blob1.size(), D3D11SW_ShaderType::Vertex);
    void* fn2 = jit.GetOrCompile(blob2.data(), blob2.size(), D3D11SW_ShaderType::Vertex);
    ASSERT_NE(fn1, nullptr);
    ASSERT_NE(fn2, nullptr);
    EXPECT_NE(fn1, fn2);
}

TEST_F(ShaderJITTests, ValidVSCallableWithZeroedInputs)
{
    auto blob = BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(MakeMinimalVSTokens()));
    void* fn = jit.GetOrCompile(blob.data(), blob.size(), D3D11SW_ShaderType::Vertex);
    ASSERT_NE(fn, nullptr);

    SW_VSInput  in  = {};
    SW_VSOutput out = {};
    SW_Resources res = {};
    auto vsFn = reinterpret_cast<SW_VSFn>(fn);
    EXPECT_NO_THROW(vsFn(&in, &out, &res));
}
