#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <algorithm>
#include <vector>
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

TEST_F(CSGoldenTests, AppendConsume8x8)
{
    constexpr UINT N = 64;

    D3D11_BUFFER_DESC sbufDesc = {};
    sbufDesc.ByteWidth           = N * 4;
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
    sbufUavDesc.Buffer.NumElements  = N;
    sbufUavDesc.Buffer.Flags        = D3D11_BUFFER_UAV_FLAG_APPEND;

    ID3D11UnorderedAccessView* sbufUav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(sbuf, &sbufUavDesc, &sbufUav)));

    auto bytecode = ReadBytecode(D3D11SW_SHADER_DIR "/cs_append_consume.o");
    ASSERT_FALSE(bytecode.empty()) << "Shader not found — run compile.sh";

    ID3D11ComputeShader* cs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateComputeShader(
        bytecode.data(), bytecode.size(), nullptr, &cs)));

    UINT initialCount = 0;
    context->CSSetShader(cs, nullptr, 0);
    context->CSSetUnorderedAccessViews(0, 1, &sbufUav, &initialCount);
    context->Dispatch(1, 1, 1);

    D3D11_BUFFER_DESC countBufDesc = {};
    countBufDesc.ByteWidth      = 4;
    countBufDesc.Usage          = D3D11_USAGE_STAGING;
    countBufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* countBuf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&countBufDesc, nullptr, &countBuf)));
    context->CopyStructureCount(countBuf, 0, sbufUav);

    D3D11_MAPPED_SUBRESOURCE countMapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(countBuf, 0, D3D11_MAP_READ, 0, &countMapped)));
    UINT counter = *static_cast<const UINT*>(countMapped.pData);
    EXPECT_EQ(counter, N) << "Counter should be 64 after 64 Append() calls";
    context->Unmap(countBuf, 0);

    D3D11_BUFFER_DESC sbufStagingDesc = sbufDesc;
    sbufStagingDesc.Usage          = D3D11_USAGE_STAGING;
    sbufStagingDesc.BindFlags      = 0;
    sbufStagingDesc.MiscFlags      = 0;
    sbufStagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* sbufStaging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&sbufStagingDesc, nullptr, &sbufStaging)));
    context->CopyResource(sbufStaging, sbuf);

    D3D11_MAPPED_SUBRESOURCE sbufMapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(sbufStaging, 0, D3D11_MAP_READ, 0, &sbufMapped)));
    auto values = static_cast<const uint32_t*>(sbufMapped.pData);
    std::vector<uint32_t> sorted(values, values + N);
    std::sort(sorted.begin(), sorted.end());
    for (uint32_t i = 0; i < N; ++i)
    {
        EXPECT_EQ(sorted[i], i) << "Missing value " << i << " in append buffer";
    }
    context->Unmap(sbufStaging, 0);

    sbufStaging->Release();
    countBuf->Release();
    sbufUav->Release();
    sbuf->Release();
    cs->Release();
}
