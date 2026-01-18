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

TEST_F(CSGoldenTests, SampleCube8x8)
{
    const UINT faceSize = 4;
    const UINT faceBytes = faceSize * faceSize * 4;
    unsigned char faces[6][faceSize * faceSize * 4];

    unsigned char faceColors[6][4] = {
        {255, 0,   0,   255},  // +X red
        {0,   255, 0,   255},  // -X green
        {0,   0,   255, 255},  // +Y blue
        {255, 255, 0,   255},  // -Y yellow
        {255, 0,   255, 255},  // +Z magenta
        {0,   255, 255, 255},  // -Z cyan
    };

    for (int f = 0; f < 6; ++f)
    {
        for (UINT i = 0; i < faceSize * faceSize; ++i)
        {
            faces[f][i * 4 + 0] = faceColors[f][0];
            faces[f][i * 4 + 1] = faceColors[f][1];
            faces[f][i * 4 + 2] = faceColors[f][2];
            faces[f][i * 4 + 3] = faceColors[f][3];
        }
    }

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width            = faceSize;
    texDesc.Height           = faceSize;
    texDesc.MipLevels        = 1;
    texDesc.ArraySize        = 6;
    texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage            = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    texDesc.MiscFlags        = D3D11_RESOURCE_MISC_TEXTURECUBE;

    D3D11_SUBRESOURCE_DATA initData[6]{};
    for (int f = 0; f < 6; ++f)
    {
        initData[f].pSysMem     = faces[f];
        initData[f].SysMemPitch = faceSize * 4;
    }

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, initData, &tex)));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels     = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(tex, &srvDesc, &srv)));

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
    uavDesc.Format        = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(dstTex, &uavDesc, &uav)));

    auto bytecode = ReadBytecode(D3D11SW_SHADER_DIR "/cs_sample_cube.o");
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
    auto result = CheckGolden("cs_sample_cube_8x8", pixels, 8, 8, 0.02f);
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
