#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <d3d11TokenizedProgramFormat.hpp>
#include "context/dispatch_executor.h"
#include "shaders/dxbc.h"
#include <vector>
#include <cstring>
#include <cstdio>
#include <atomic>

using namespace d3d11sw;


static std::atomic<int> gInvocationCount{0};
static SW_CSInput        gLastInput{};

static void MockCS(const SW_CSInput* input, SW_Resources* res)
{
    ++gInvocationCount;
    gLastInput = *input;
}

struct DispatchExecutorTests : ::testing::Test
{
    SingleThreadedDispatchExecutor exec;
    SW_Resources                   res{};

    void SetUp() override
    {
        gInvocationCount = 0;
        gLastInput       = {};
    }
};

TEST_F(DispatchExecutorTests, InvocationCount)
{
    exec.DispatchCS(2, 3, 1,  
                    4, 1, 1,  
                    MockCS, res);
    EXPECT_EQ(gInvocationCount, 2 * 3 * 1 * 4 * 1 * 1);
}

TEST_F(DispatchExecutorTests, InvocationCount3D)
{
    exec.DispatchCS(2, 2, 2,
                    4, 4, 4,
                    MockCS, res);
    EXPECT_EQ(gInvocationCount, 2 * 2 * 2 * 4 * 4 * 4);
}

TEST_F(DispatchExecutorTests, GroupID)
{
    std::vector<SW_uint3> groupIDs;
    SW_Resources r{};

    auto fn = [](const SW_CSInput* input, SW_Resources*) {
        (void)input;
    };

    struct Capture { std::vector<SW_uint3>* ids; };
    static Capture cap;
    static std::vector<SW_uint3> captured;
    captured.clear();
    cap.ids = &captured;

    auto captureFn = [](const SW_CSInput* input, SW_Resources*) {
        if (input->groupThreadID.x == 0 &&
            input->groupThreadID.y == 0 &&
            input->groupThreadID.z == 0)
        {
            cap.ids->push_back(input->groupID);
        }
    };

    exec.DispatchCS(2, 2, 1, 1, 1, 1, captureFn, res);

    ASSERT_EQ(captured.size(), 4u);
    EXPECT_EQ(captured[0].x, 0u); EXPECT_EQ(captured[0].y, 0u);
    EXPECT_EQ(captured[1].x, 1u); EXPECT_EQ(captured[1].y, 0u);
    EXPECT_EQ(captured[2].x, 0u); EXPECT_EQ(captured[2].y, 1u);
    EXPECT_EQ(captured[3].x, 1u); EXPECT_EQ(captured[3].y, 1u);
}

TEST_F(DispatchExecutorTests, DispatchThreadID)
{
    static SW_uint3 lastDispatchID{};

    auto fn = [](const SW_CSInput* input, SW_Resources*) {
        lastDispatchID = input->dispatchThreadID;
    };

    exec.DispatchCS(1, 1, 1, 4, 1, 1, fn, res);

    EXPECT_EQ(lastDispatchID.x, 3u);
    EXPECT_EQ(lastDispatchID.y, 0u);
    EXPECT_EQ(lastDispatchID.z, 0u);
}

TEST_F(DispatchExecutorTests, DispatchThreadIDMultiGroup)
{
    static SW_uint3 lastDispatchID{};

    auto fn = [](const SW_CSInput* input, SW_Resources*) {
        lastDispatchID = input->dispatchThreadID;
    };

    exec.DispatchCS(2, 1, 1, 4, 1, 1, fn, res);

    EXPECT_EQ(lastDispatchID.x, 7u);
}

TEST_F(DispatchExecutorTests, ZeroGroupsNoInvocations)
{
    exec.DispatchCS(0, 1, 1, 4, 1, 1, MockCS, res);
    EXPECT_EQ(gInvocationCount, 0);
}

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

// Bytecode from https://shader-playground.timjones.io
// RWBuffer<float4> output : register(u0);
// [numthreads(8, 8, 1)]
// void main(uint3 DTid : SV_DispatchThreadID)
// {
//     uint idx = DTid.y * 8 + DTid.x;
//     output[idx] = float4(DTid.x / 7.0, DTid.y / 7.0, 0.5, 1.0);
// }
static const unsigned char kGradientCS[] = {
  0x44, 0x58, 0x42, 0x43, 0x63, 0x59, 0x2c, 0xaa, 0x09, 0x69, 0xb1, 0x56,
  0x24, 0xc0, 0x9a, 0xe7, 0x1d, 0x01, 0xd8, 0x12, 0x01, 0x00, 0x00, 0x00,
  0x7c, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00,
  0xd4, 0x00, 0x00, 0x00, 0xe4, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00,
  0xe0, 0x01, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x98, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x3c, 0x00, 0x00, 0x00, 0x00, 0x05, 0x53, 0x43, 0x00, 0x01, 0x00, 0x00,
  0x63, 0x00, 0x00, 0x00, 0x52, 0x44, 0x31, 0x31, 0x3c, 0x00, 0x00, 0x00,
  0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
  0x24, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x5c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x6f, 0x75, 0x74, 0x70,
  0x75, 0x74, 0x00, 0x4d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74,
  0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4c, 0x53, 0x4c, 0x20, 0x53, 0x68,
  0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6f, 0x6d, 0x70, 0x69, 0x6c, 0x65,
  0x72, 0x20, 0x31, 0x30, 0x2e, 0x30, 0x2e, 0x31, 0x30, 0x30, 0x31, 0x31,
  0x2e, 0x31, 0x36, 0x33, 0x38, 0x34, 0x00, 0xab, 0x49, 0x53, 0x47, 0x4e,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x4f, 0x53, 0x47, 0x4e, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x53, 0x48, 0x45, 0x58, 0xe4, 0x00, 0x00, 0x00,
  0x50, 0x00, 0x05, 0x00, 0x39, 0x00, 0x00, 0x00, 0x6a, 0x08, 0x00, 0x01,
  0x9c, 0x08, 0x00, 0x04, 0x00, 0xe0, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x55, 0x55, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x02, 0x32, 0x00, 0x02, 0x00,
  0x68, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00, 0x9b, 0x00, 0x00, 0x04,
  0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x29, 0x00, 0x00, 0x06, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x1a, 0x00, 0x02, 0x00, 0x01, 0x40, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x1e, 0x00, 0x00, 0x06, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0a, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x02, 0x00,
  0x56, 0x00, 0x00, 0x04, 0x62, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x06, 0x01, 0x02, 0x00, 0x38, 0x00, 0x00, 0x0a, 0x32, 0x00, 0x10, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x96, 0x05, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x40, 0x00, 0x00, 0x25, 0x49, 0x12, 0x3e, 0x25, 0x49, 0x12, 0x3e,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x08,
  0xc2, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f,
  0x00, 0x00, 0x80, 0x3f, 0xa4, 0x00, 0x00, 0x07, 0xf2, 0xe0, 0x11, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x46, 0x0e, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x01,
  0x53, 0x54, 0x41, 0x54, 0x94, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
};

TEST_F(DispatchIntegrationTests, GradientCSWritesCorrectPixels)
{
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.ByteWidth           = 64 * 16;
    bufDesc.Usage               = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS;
    bufDesc.CPUAccessFlags      = 0;
    bufDesc.MiscFlags           = 0;
    bufDesc.StructureByteStride = 0;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format              = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements  = 64;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

    ID3D11ComputeShader* cs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateComputeShader(
        kGradientCS, sizeof(kGradientCS), nullptr, &cs)));

    context->CSSetShader(cs, nullptr, 0);
    context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

    // Dispatch 1x1x1 group of 8x8x1 threads
    context->Dispatch(1, 1, 1);

    D3D11_BUFFER_DESC stagingDesc = bufDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, buf);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    auto pixels = static_cast<const float*>(mapped.pData);

    auto get = [&](int x, int y) -> const float* {
        return pixels + (y * 8 + x) * 4;
    };

    // pixel(0,0) = {0, 0, 0.5, 1.0}
    EXPECT_NEAR(get(0,0)[0], 0.f,   0.01f);
    EXPECT_NEAR(get(0,0)[1], 0.f,   0.01f);
    EXPECT_NEAR(get(0,0)[2], 0.5f,  0.01f);
    EXPECT_NEAR(get(0,0)[3], 1.0f,  0.01f);

    // pixel(7,0) = {1, 0, 0.5, 1.0}
    EXPECT_NEAR(get(7,0)[0], 1.0f,  0.01f);
    EXPECT_NEAR(get(7,0)[1], 0.f,   0.01f);

    // pixel(0,7) = {0, 1, 0.5, 1.0}
    EXPECT_NEAR(get(0,7)[0], 0.f,   0.01f);
    EXPECT_NEAR(get(0,7)[1], 1.0f,  0.01f);

    // pixel(7,7) = {1, 1, 0.5, 1.0}
    EXPECT_NEAR(get(7,7)[0], 1.0f,  0.01f);
    EXPECT_NEAR(get(7,7)[1], 1.0f,  0.01f);
    EXPECT_NEAR(get(7,7)[2], 0.5f,  0.01f);
    EXPECT_NEAR(get(7,7)[3], 1.0f,  0.01f);

    context->Unmap(staging, 0);

    std::FILE* f = std::fopen("/tmp/d3d11sw/gradient_test.ppm", "wb");
    if (f)
    {
        std::fprintf(f, "P6\n8 8\n255\n");
        for (int y = 0; y < 8; ++y)
        {
            for (int x = 0; x < 8; ++x)
            {
                const float* p = get(x, y);
                unsigned char rgb[3] = {
                    (unsigned char)(p[0] * 255.f + 0.5f),
                    (unsigned char)(p[1] * 255.f + 0.5f),
                    (unsigned char)(p[2] * 255.f + 0.5f)
                };
                std::fwrite(rgb, 1, 3, f);
            }
        }
        std::fclose(f);
    }

    staging->Release();
    uav->Release();
    buf->Release();
    cs->Release();
}
