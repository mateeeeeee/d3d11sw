#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "perf_util.h"
#include "../golden/golden_test_util.h"

struct PerfCompute : ::testing::Test
{
    ID3D11Device*              device  = nullptr;
    ID3D11DeviceContext*       context = nullptr;
    ID3D11ComputeShader*       cs      = nullptr;
    ID3D11Buffer*              buf     = nullptr;
    ID3D11UnorderedAccessView* uav     = nullptr;

    static constexpr UINT kMaxElements = 16384;

    void SetUp() override
    {
        D3D_FEATURE_LEVEL fl;
        ASSERT_TRUE(SUCCEEDED(D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION, &device, &fl, &context)));

        D3D11_BUFFER_DESC bufDesc{};
        bufDesc.ByteWidth = kMaxElements * 16;
        bufDesc.Usage     = D3D11_USAGE_DEFAULT;
        bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format              = DXGI_FORMAT_R32G32B32A32_FLOAT;
        uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.NumElements  = kMaxElements;
        ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

        auto bytecode = ReadBytecode(D3D11SW_SHADER_DIR "/cs_solid_fill.o");
        ASSERT_FALSE(bytecode.empty()) << "cs_solid_fill.o not found — run compile.sh";
        ASSERT_TRUE(SUCCEEDED(device->CreateComputeShader(
            bytecode.data(), bytecode.size(), nullptr, &cs)));

        context->CSSetShader(cs, nullptr, 0);
        context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
    }

    void TearDown() override
    {
        if (uav)     { uav->Release(); }
        if (buf)     { buf->Release(); }
        if (cs)      { cs->Release(); }
        if (context) { context->Release(); }
        if (device)  { device->Release(); }
    }
};

TEST_F(PerfCompute, SingleGroup_8x8x1)
{
    auto result = RunBenchmark("SingleGroup_8x8x1", context, device, 100, 10, [&]() {
        context->Dispatch(1, 1, 1);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
}

TEST_F(PerfCompute, MultiGroup_4x4)
{
    auto result = RunBenchmark("MultiGroup_4x4", context, device, 100, 10, [&]() {
        context->Dispatch(4, 4, 1);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
}

TEST_F(PerfCompute, MultiGroup_16x16)
{
    auto result = RunBenchmark("MultiGroup_16x16", context, device, 100, 10, [&]() {
        context->Dispatch(16, 16, 1);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
}

TEST_F(PerfCompute, LargeDispatch_128x128)
{
    auto result = RunBenchmark("LargeDispatch_128x128", context, device, 3, 1, [&]() {
        context->Dispatch(128, 128, 1);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
}
