#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <d3d11TokenizedProgramFormat.hpp>
#include <atomic>
#include "context/dispatcher.h"
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

    Uint32 magic = MakeFourCC('D','X','B','C'); Uint32 version = 1; Uint32 chunkCount = 1;
    std::memcpy(p, &magic,       4); p += 4;
    p += 16;
    std::memcpy(p, &version,     4); p += 4;
    std::memcpy(p, &totalSize,   4); p += 4;
    std::memcpy(p, &chunkCount,  4); p += 4;
    std::memcpy(p, &chunkOffset, 4); p += 4;
    std::memcpy(p, &chunkFourCC, 4); p += 4;
    std::memcpy(p, &chunkSize,   4); p += 4;
    std::memcpy(p, payload.data(), chunkSize);
    return blob;
}

static std::vector<Uint8> MakeMinimalCSBlob()
{
    std::vector<Uint32> tokens = {
        0xFFFF0500,
        0x00000006,
        ENCODE_D3D10_SB_OPCODE_TYPE(D3D11_SB_OPCODE_DCL_THREAD_GROUP) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(4),
        0x00000004,  // X = 4
        0x00000001,  // Y = 1
        0x00000001,  // Z = 1
        ENCODE_D3D10_SB_OPCODE_TYPE(D3D10_SB_OPCODE_RET) | ENCODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(1),
    };
    std::vector<Uint8> payload(tokens.size() * 4);
    std::memcpy(payload.data(), tokens.data(), payload.size());
    return BuildDXBCBlob(FOURCC_SHDR, payload);
}

struct DispatchIntegrationTests : ::testing::Test
{
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* context = nullptr;

    void SetUp() override
    {
        D3D_FEATURE_LEVEL fl;
        ASSERT_TRUE(SUCCEEDED(D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION, &device, &fl, &context)));
    }

    void TearDown() override
    {
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }
};

TEST_F(DispatchIntegrationTests, DispatchWithNoCSBoundDoesNotCrash)
{
    context->Dispatch(1, 1, 1);
}

TEST_F(DispatchIntegrationTests, DispatchWithValidCSDoesNotCrash)
{
    auto blob = MakeMinimalCSBlob();
    ID3D11ComputeShader* cs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateComputeShader(blob.data(), blob.size(), nullptr, &cs)));

    context->CSSetShader(cs, nullptr, 0);
    context->Dispatch(2, 1, 1);

    cs->Release();
}
