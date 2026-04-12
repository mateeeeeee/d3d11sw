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

// BC1 block: 2 RGB565 endpoints + 16 2-bit selectors (4 bytes).
// All selectors 0 → every pixel = color0.
static void MakeSolidBC1Block(uint8_t out[8], uint16_t color0, uint16_t color1 = 0)
{
    // Ensure 4-color mode: color0 must be > color1
    if (color0 <= color1) color1 = 0;
    out[0] = (uint8_t)(color0);
    out[1] = (uint8_t)(color0 >> 8);
    out[2] = (uint8_t)(color1);
    out[3] = (uint8_t)(color1 >> 8);
    out[4] = out[5] = out[6] = out[7] = 0; // all selectors = 0
}

TEST_F(CSGoldenTests, BC1Texture)
{
    // 8x8 BC1_UNORM texture = 2x2 blocks, each block is 8 bytes = 32 bytes total
    // Block layout:
    //   (0,0) red     (1,0) green
    //   (0,1) blue    (1,1) white
    const UINT W = 8, H = 8;
    const UINT blocksX = 2, blocksY = 2;
    const UINT blockSize = 8;
    const UINT rowPitch = blocksX * blockSize; // 16

    uint8_t bcData[32];
    MakeSolidBC1Block(bcData + 0,  0xF800); // red:   R=31,G=0, B=0
    MakeSolidBC1Block(bcData + 8,  0x07E0); // green: R=0, G=63,B=0
    MakeSolidBC1Block(bcData + 16, 0x001F); // blue:  R=0, G=0, B=31
    MakeSolidBC1Block(bcData + 24, 0xFFFF); // white: R=31,G=63,B=31

    D3D11_TEXTURE2D_DESC srcDesc = {};
    srcDesc.Width            = W;
    srcDesc.Height           = H;
    srcDesc.MipLevels        = 1;
    srcDesc.ArraySize        = 1;
    srcDesc.Format           = DXGI_FORMAT_BC1_UNORM;
    srcDesc.SampleDesc.Count = 1;
    srcDesc.Usage            = D3D11_USAGE_DEFAULT;
    srcDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem     = bcData;
    initData.SysMemPitch = rowPitch;

    ID3D11Texture2D* srcTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&srcDesc, &initData, &srcTex)));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                          = DXGI_FORMAT_BC1_UNORM;
    srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip       = 0;
    srvDesc.Texture2D.MipLevels             = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(srcTex, &srvDesc, &srv)));

    // 8x8 R32G32B32A32_FLOAT output
    D3D11_TEXTURE2D_DESC dstDesc = {};
    dstDesc.Width            = W;
    dstDesc.Height           = H;
    dstDesc.MipLevels        = 1;
    dstDesc.ArraySize        = 1;
    dstDesc.Format           = DXGI_FORMAT_R32G32B32A32_FLOAT;
    dstDesc.SampleDesc.Count = 1;
    dstDesc.Usage            = D3D11_USAGE_DEFAULT;
    dstDesc.BindFlags        = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Texture2D* dstTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&dstDesc, nullptr, &dstTex)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format                           = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension                    = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice               = 0;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(dstTex, &uavDesc, &uav)));

    // Reuse cs_texture shader (does src.Load → dst write)
    auto bytecode = ReadBytecode(D3D11SW_SHADER_DIR "/cs_texture.o");
    ASSERT_FALSE(bytecode.empty()) << "Shader not found — run compile.sh";

    ID3D11ComputeShader* cs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateComputeShader(
        bytecode.data(), bytecode.size(), nullptr, &cs)));

    context->CSSetShader(cs, nullptr, 0);
    context->CSSetShaderResources(0, 1, &srv);
    context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
    context->Dispatch(1, 1, 1);

    D3D11_TEXTURE2D_DESC stagingDesc = dstDesc;
    stagingDesc.Usage                = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags            = 0;
    stagingDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, dstTex);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    auto pixels = static_cast<const float*>(mapped.pData);

    // Verify each 4x4 quadrant is a solid color
    auto checkQuadrant = [&](int bx, int by, float er, float eg, float eb, const char* name) {
        for (int ly = 0; ly < 4; ++ly)
        {
            for (int lx = 0; lx < 4; ++lx)
            {
                int x = bx * 4 + lx;
                int y = by * 4 + ly;
                int idx = (y * W + x) * 4;
                EXPECT_NEAR(pixels[idx + 0], er, 0.01f) << name << " R at (" << x << "," << y << ")";
                EXPECT_NEAR(pixels[idx + 1], eg, 0.01f) << name << " G at (" << x << "," << y << ")";
                EXPECT_NEAR(pixels[idx + 2], eb, 0.01f) << name << " B at (" << x << "," << y << ")";
                EXPECT_NEAR(pixels[idx + 3], 1.0f, 0.01f) << name << " A at (" << x << "," << y << ")";
            }
        }
    };

    checkQuadrant(0, 0, 1.0f, 0.0f, 0.0f, "red");
    checkQuadrant(1, 0, 0.0f, 1.0f, 0.0f, "green");
    checkQuadrant(0, 1, 0.0f, 0.0f, 1.0f, "blue");
    checkQuadrant(1, 1, 1.0f, 1.0f, 1.0f, "white");

    auto result = CheckGolden("cs_bc1_texture_8x8", pixels, W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    context->Unmap(staging, 0);

    staging->Release();
    uav->Release();
    dstTex->Release();
    cs->Release();
    srv->Release();
    srcTex->Release();
}
