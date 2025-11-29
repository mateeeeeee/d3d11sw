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

TEST_F(CSGoldenTests, Gather8x8)
{
    // 8x8 R8G8B8A8_UNORM texture: R = x*32, G = y*32, B = 128, A = 255
    unsigned char texData[8 * 8 * 4];
    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            int i          = (y * 8 + x) * 4;
            texData[i + 0] = (unsigned char)(x * 32);
            texData[i + 1] = (unsigned char)(y * 32);
            texData[i + 2] = 128;
            texData[i + 3] = 255;
        }
    }

    D3D11_TEXTURE2D_DESC srcDesc   = {};
    srcDesc.Width                  = 8;
    srcDesc.Height                 = 8;
    srcDesc.MipLevels              = 1;
    srcDesc.ArraySize              = 1;
    srcDesc.Format                 = DXGI_FORMAT_R8G8B8A8_UNORM;
    srcDesc.SampleDesc.Count       = 1;
    srcDesc.Usage                  = D3D11_USAGE_DEFAULT;
    srcDesc.BindFlags              = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem                = texData;
    initData.SysMemPitch            = 8 * 4;

    ID3D11Texture2D* srcTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&srcDesc, &initData, &srcTex)));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc          = {};
    srvDesc.Format                                   = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension                            = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip                = 0;
    srvDesc.Texture2D.MipLevels                      = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(srcTex, &srvDesc, &srv)));

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU           = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV           = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW           = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.MinLOD             = 0.f;
    sampDesc.MaxLOD             = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* smp = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateSamplerState(&sampDesc, &smp)));

    D3D11_TEXTURE2D_DESC dstDesc   = {};
    dstDesc.Width                  = 8;
    dstDesc.Height                 = 8;
    dstDesc.MipLevels              = 1;
    dstDesc.ArraySize              = 1;
    dstDesc.Format                 = DXGI_FORMAT_R32G32B32A32_FLOAT;
    dstDesc.SampleDesc.Count       = 1;
    dstDesc.Usage                  = D3D11_USAGE_DEFAULT;
    dstDesc.BindFlags              = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Texture2D* dstTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&dstDesc, nullptr, &dstTex)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc  = {};
    uavDesc.Format                            = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension                     = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice                = 0;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(dstTex, &uavDesc, &uav)));

    auto bytecode = ReadBytecode(D3D11SW_SHADER_DIR "/cs_gather.o");
    ASSERT_FALSE(bytecode.empty()) << "Shader not found — run compile.sh";

    ID3D11ComputeShader* cs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateComputeShader(
        bytecode.data(), bytecode.size(), nullptr, &cs)));

    context->CSSetShader(cs, nullptr, 0);
    context->CSSetShaderResources(0, 1, &srv);
    context->CSSetSamplers(0, 1, &smp);
    context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
    context->Dispatch(1, 1, 1);

    D3D11_TEXTURE2D_DESC stagingDesc  = dstDesc;
    stagingDesc.Usage                 = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags             = 0;
    stagingDesc.CPUAccessFlags        = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, dstTex);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    auto pixels = static_cast<const float*>(mapped.pData);
    auto result = CheckGolden("cs_gather_8x8", pixels, 8, 8, 0.01f);
    EXPECT_TRUE(result.passed) << result.message;

    context->Unmap(staging, 0);

    staging->Release();
    uav->Release();
    dstTex->Release();
    cs->Release();
    smp->Release();
    srv->Release();
    srcTex->Release();
}
