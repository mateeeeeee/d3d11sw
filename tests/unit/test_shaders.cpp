#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <d3d11TokenizedProgramFormat.hpp>
#include "shaders/vertex_shader.h"
#include "shaders/pixel_shader.h"
#include "shaders/compute_shader.h"
#include "shaders/dxbc.h"
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

static std::vector<Uint8> MakeMinimalSHDRBlob()
{
    std::vector<Uint32> tokens = {
        0xFFFE0400,
        0x00000004,
        ENCODE_D3D10_SB_OPCODE_TYPE(D3D10_SB_OPCODE_DCL_TEMPS) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(2),
        0x00000001,
        ENCODE_D3D10_SB_OPCODE_TYPE(D3D10_SB_OPCODE_RET) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(1),
    };
    return BuildDXBCBlob(FOURCC_SHDR, DwordsToBytes(tokens));
}

struct ShaderTests : ::testing::Test
{
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* context = nullptr;

    void SetUp() override
    {
        D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &device, &featureLevel, &context);
        ASSERT_TRUE(SUCCEEDED(hr));
        ASSERT_NE(device, nullptr);
    }

    void TearDown() override
    {
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }
};

TEST_F(ShaderTests, CreateVertexShaderSucceeds)
{
    auto blob = MakeMinimalSHDRBlob();
    ID3D11VertexShader* shader = nullptr;
    HRESULT hr = device->CreateVertexShader(blob.data(), blob.size(), nullptr, &shader);
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(shader, nullptr);
    if (shader) { shader->Release(); }
}

TEST_F(ShaderTests, CreatePixelShaderSucceeds)
{
    auto blob = MakeMinimalSHDRBlob();
    ID3D11PixelShader* shader = nullptr;
    HRESULT hr = device->CreatePixelShader(blob.data(), blob.size(), nullptr, &shader);
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(shader, nullptr);
    if (shader) { shader->Release(); }
}

TEST_F(ShaderTests, CreateComputeShaderSucceeds)
{
    auto blob = MakeMinimalSHDRBlob();
    ID3D11ComputeShader* shader = nullptr;
    HRESULT hr = device->CreateComputeShader(blob.data(), blob.size(), nullptr, &shader);
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(shader, nullptr);
    if (shader) { shader->Release(); }
}

TEST_F(ShaderTests, CreateVertexShaderNullOutputReturnsSFalse)
{
    auto blob = MakeMinimalSHDRBlob();
    HRESULT hr = device->CreateVertexShader(blob.data(), blob.size(), nullptr, nullptr);
    EXPECT_EQ(hr, S_FALSE);
}

TEST_F(ShaderTests, CreatePixelShaderNullOutputReturnsSFalse)
{
    auto blob = MakeMinimalSHDRBlob();
    HRESULT hr = device->CreatePixelShader(blob.data(), blob.size(), nullptr, nullptr);
    EXPECT_EQ(hr, S_FALSE);
}

TEST_F(ShaderTests, CreateComputeShaderNullOutputReturnsSFalse)
{
    auto blob = MakeMinimalSHDRBlob();
    HRESULT hr = device->CreateComputeShader(blob.data(), blob.size(), nullptr, nullptr);
    EXPECT_EQ(hr, S_FALSE);
}

TEST_F(ShaderTests, CreateVertexShaderInvalidBytecodeFails)
{
    Uint8 garbage[64] = {};
    ID3D11VertexShader* shader = nullptr;
    HRESULT hr = device->CreateVertexShader(garbage, sizeof(garbage), nullptr, &shader);
    EXPECT_TRUE(FAILED(hr));
    EXPECT_EQ(shader, nullptr);
}
