#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "golden_test_util.h"

struct CSGoldenTests : ::testing::Test
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

TEST_F(CSGoldenTests, Structured8x8)
{
    //RWStructuredBuffer<uint>
    D3D11_BUFFER_DESC sbufDesc = {};
    sbufDesc.ByteWidth           = 64 * 4;  // 64 uints
    sbufDesc.Usage               = D3D11_USAGE_DEFAULT;
    sbufDesc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS;
    sbufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    sbufDesc.StructureByteStride = 4;

    ID3D11Buffer* sbuf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&sbufDesc, nullptr, &sbuf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC sbufUavDesc = {};
    sbufUavDesc.Format              = DXGI_FORMAT_UNKNOWN;
    sbufUavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    sbufUavDesc.Buffer.FirstElement = 0;
    sbufUavDesc.Buffer.NumElements  = 64;

    ID3D11UnorderedAccessView* sbufUav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(sbuf, &sbufUavDesc, &sbufUav)));

    D3D11_BUFFER_DESC outDesc = {};
    outDesc.ByteWidth           = 64 * 16;
    outDesc.Usage               = D3D11_USAGE_DEFAULT;
    outDesc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Buffer* outBuf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&outDesc, nullptr, &outBuf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC outUavDesc = {};
    outUavDesc.Format              = DXGI_FORMAT_R32G32B32A32_FLOAT;
    outUavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    outUavDesc.Buffer.FirstElement = 0;
    outUavDesc.Buffer.NumElements  = 64;

    ID3D11UnorderedAccessView* outUav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(outBuf, &outUavDesc, &outUav)));

    auto bytecode = ReadBytecode(D3D11SW_SHADER_DIR "/cs_structured.o");
    ASSERT_FALSE(bytecode.empty()) << "Shader not found — run compile.sh";

    ID3D11ComputeShader* cs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateComputeShader(
        bytecode.data(), bytecode.size(), nullptr, &cs)));

    ID3D11UnorderedAccessView* uavs[2] = { sbufUav, outUav };
    context->CSSetShader(cs, nullptr, 0);
    context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
    context->Dispatch(1, 1, 1);

    D3D11_BUFFER_DESC stagingDesc = outDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, outBuf);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    auto pixels = static_cast<const float*>(mapped.pData);

    auto result = CheckGolden("cs_structured_8x8", pixels, 8, 8, 0.01f);
    EXPECT_TRUE(result.passed) << result.message;

    context->Unmap(staging, 0);

    staging->Release();
    outUav->Release();
    outBuf->Release();
    sbufUav->Release();
    sbuf->Release();
    cs->Release();
}
