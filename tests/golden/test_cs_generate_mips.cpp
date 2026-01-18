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

TEST_F(CSGoldenTests, GenerateMips8x8)
{
    // 8x8 checkerboard: red and blue in 2x2 blocks
    unsigned char mip0[8 * 8 * 4];
    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            int i = (y * 8 + x) * 4;
            bool red = ((x / 2) + (y / 2)) % 2 == 0;
            mip0[i + 0] = red ? 255 : 0;
            mip0[i + 1] = 0;
            mip0[i + 2] = red ? 0 : 255;
            mip0[i + 3] = 255;
        }
    }

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width            = 8;
    texDesc.Height           = 8;
    texDesc.MipLevels        = 0; // auto-compute all mips (4 levels: 8,4,2,1)
    texDesc.ArraySize        = 1;
    texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage            = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    texDesc.MiscFlags        = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &tex)));
    context->UpdateSubresource(tex, 0, nullptr, mip0, 8 * 4, 0);

    ID3D11ShaderResourceView* srv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(tex, nullptr, &srv)));

    context->GenerateMips(srv);

    // Read back all 4 mip levels into a single 8x8 output:
    //   top-left 4x4: mip1 (4x4)
    //   top-right 2x2: mip2 (2x2)
    //   bottom-left 1x1: mip3 (1x1)
    //   rest: black

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
    uavDesc.Format        = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(dstTex, &uavDesc, &uav)));

    // Use a CS to copy mip levels into the output
    auto bytecode = ReadBytecode(D3D11SW_SHADER_DIR "/cs_read_mips.o");
    ASSERT_FALSE(bytecode.empty()) << "Shader not found — run compile.sh";

    ID3D11ComputeShader* cs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateComputeShader(
        bytecode.data(), bytecode.size(), nullptr, &cs)));

    D3D11_SAMPLER_DESC smpDesc{};
    smpDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
    smpDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    smpDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    smpDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    smpDesc.MaxLOD   = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* sampler = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateSamplerState(&smpDesc, &sampler)));

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
    auto result = CheckGolden("cs_generate_mips_8x8", pixels, 8, 8, 0.02f);
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
