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

TEST_F(CSGoldenTests, Sample3d8x8)
{
    unsigned char slice0[2 * 2 * 4];
    unsigned char slice1[2 * 2 * 4];

    for (UINT i = 0; i < 4; ++i)
    {
        slice0[i * 4 + 0] = 255; slice0[i * 4 + 1] = 0;   slice0[i * 4 + 2] = 0;   slice0[i * 4 + 3] = 255;
        slice1[i * 4 + 0] = 0;   slice1[i * 4 + 1] = 0;   slice1[i * 4 + 2] = 255; slice1[i * 4 + 3] = 255;
    }

    D3D11_TEXTURE3D_DESC texDesc{};
    texDesc.Width     = 2;
    texDesc.Height    = 2;
    texDesc.Depth     = 2;
    texDesc.MipLevels = 1;
    texDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.Usage     = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    unsigned char texData[2 * 2 * 2 * 4];
    std::memcpy(texData, slice0, sizeof(slice0));
    std::memcpy(texData + sizeof(slice0), slice1, sizeof(slice1));

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem          = texData;
    initData.SysMemPitch      = 2 * 4;
    initData.SysMemSlicePitch = 2 * 2 * 4;

    ID3D11Texture3D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture3D(&texDesc, &initData, &tex)));

    ID3D11ShaderResourceView* srv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(tex, nullptr, &srv)));

    D3D11_SAMPLER_DESC smpDesc{};
    smpDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
    smpDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    smpDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    smpDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    smpDesc.MaxLOD   = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* sampler = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateSamplerState(&smpDesc, &sampler)));

    D3D11_TEXTURE2D_DESC dstDesc{};
    dstDesc.Width            = 8;
    dstDesc.Height           = 8;
    dstDesc.MipLevels        = 1;
    dstDesc.ArraySize        = 1;
    dstDesc.Format           = DXGI_FORMAT_R32G32B32A32_FLOAT;
    dstDesc.SampleDesc.Count = 1;
    dstDesc.Usage            = D3D11_USAGE_DEFAULT;
    dstDesc.BindFlags        = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Texture2D* dstTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&dstDesc, nullptr, &dstTex)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format             = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(dstTex, &uavDesc, &uav)));

    auto bytecode = ReadBytecode(D3D11SW_SHADER_DIR "/cs_sample_3d.o");
    ASSERT_FALSE(bytecode.empty()) << "Shader not found — run compile.sh";

    ID3D11ComputeShader* cs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateComputeShader(
        bytecode.data(), bytecode.size(), nullptr, &cs)));

    context->CSSetShader(cs, nullptr, 0);
    context->CSSetShaderResources(0, 1, &srv);
    context->CSSetSamplers(0, 1, &sampler);
    context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
    context->Dispatch(1, 1, 1);

    D3D11_TEXTURE2D_DESC stagingDesc = dstDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, dstTex);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    auto pixels = static_cast<const float*>(mapped.pData);
    auto result = CheckGolden("cs_sample_3d_8x8", pixels, 8, 8, 0.02f);
    EXPECT_TRUE(result.passed) << result.message;

    context->Unmap(staging, 0);
    staging->Release();
    uav->Release();
    dstTex->Release();
    cs->Release();
    sampler->Release();
    srv->Release();
    tex->Release();
}
