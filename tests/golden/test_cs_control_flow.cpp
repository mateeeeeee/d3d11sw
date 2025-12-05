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

TEST_F(CSGoldenTests, ControlFlow8x8)
{
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.ByteWidth = 64 * 16;
    bufDesc.Usage     = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format              = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements  = 64;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

    auto bytecode = ReadBytecode(D3D11SW_SHADER_DIR "/cs_control_flow.o");
    ASSERT_FALSE(bytecode.empty()) << "Shader not found — run compile.sh";

    ID3D11ComputeShader* cs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateComputeShader(
        bytecode.data(), bytecode.size(), nullptr, &cs)));

    context->CSSetShader(cs, nullptr, 0);
    context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
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
    auto result = CheckGolden("cs_control_flow_8x8", pixels, 8, 8, 0.01f);
    EXPECT_TRUE(result.passed) << result.message;

    // Sanity: top-left quadrant should be pure red
    EXPECT_GT(pixels[0], 0.9f); // (0,0).r should be ~1.0
    EXPECT_LT(pixels[1], 0.1f); // (0,0).g should be ~0.0

    context->Unmap(staging, 0);
    staging->Release();
    uav->Release();
    buf->Release();
    cs->Release();
}
