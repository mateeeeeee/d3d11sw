#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "golden_test_util.h"

using namespace d3d11sw;

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

    void RunAddressTest(const char* name, D3D11_TEXTURE_ADDRESS_MODE addrU,
                        D3D11_TEXTURE_ADDRESS_MODE addrV, const FLOAT borderColor[4])
    {
        const UINT W = 8, H = 8;

        Uint8 texData[4 * 4 * 4];
        for (UINT y = 0; y < 4; ++y)
        {
            for (UINT x = 0; x < 4; ++x)
            {
                Uint8* p = texData + (y * 4 + x) * 4;
                if ((x + y) & 1)
                {
                    p[0] = 0; p[1] = 255; p[2] = 0; p[3] = 255;
                }
                else
                {
                    p[0] = 255; p[1] = 0; p[2] = 0; p[3] = 255;
                }
            }
        }

        D3D11_TEXTURE2D_DESC srcDesc{};
        srcDesc.Width            = 4;
        srcDesc.Height           = 4;
        srcDesc.MipLevels        = 1;
        srcDesc.ArraySize        = 1;
        srcDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        srcDesc.SampleDesc.Count = 1;
        srcDesc.Usage            = D3D11_USAGE_DEFAULT;
        srcDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA srcInit{};
        srcInit.pSysMem     = texData;
        srcInit.SysMemPitch = 4 * 4;

        ID3D11Texture2D* srcTex = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&srcDesc, &srcInit, &srcTex)));

        ID3D11ShaderResourceView* srv = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(srcTex, nullptr, &srv)));

        D3D11_TEXTURE2D_DESC dstDesc{};
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

        ID3D11UnorderedAccessView* uav = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(dstTex, nullptr, &uav)));

        D3D11_SAMPLER_DESC smpDesc{};
        smpDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
        smpDesc.AddressU = addrU;
        smpDesc.AddressV = addrV;
        smpDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        smpDesc.MaxLOD   = D3D11_FLOAT32_MAX;
        if (borderColor)
        {
            smpDesc.BorderColor[0] = borderColor[0];
            smpDesc.BorderColor[1] = borderColor[1];
            smpDesc.BorderColor[2] = borderColor[2];
            smpDesc.BorderColor[3] = borderColor[3];
        }

        ID3D11SamplerState* sampler = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateSamplerState(&smpDesc, &sampler)));

        auto csBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/cs_sample_addr.o");
        ASSERT_FALSE(csBytecode.empty()) << "cs_sample_addr.o not found — run compile.sh";

        ID3D11ComputeShader* cs = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateComputeShader(
            csBytecode.data(), csBytecode.size(), nullptr, &cs)));

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

        std::vector<float> pixels(W * H * 4);
        for (UINT y = 0; y < H; ++y)
        {
            const float* row = reinterpret_cast<const float*>(
                static_cast<const Uint8*>(mapped.pData) + y * mapped.RowPitch);
            for (UINT x = 0; x < W; ++x)
            {
                UINT idx = (y * W + x) * 4;
                pixels[idx + 0] = row[x * 4 + 0];
                pixels[idx + 1] = row[x * 4 + 1];
                pixels[idx + 2] = row[x * 4 + 2];
                pixels[idx + 3] = row[x * 4 + 3];
            }
        }
        context->Unmap(staging, 0);

        auto result = CheckGolden(name, pixels.data(), W, H, 0.01f);
        EXPECT_TRUE(result.passed) << result.message;

        staging->Release();
        cs->Release();
        sampler->Release();
        uav->Release();
        dstTex->Release();
        srv->Release();
        srcTex->Release();
    }
};

TEST_F(CSGoldenTests, AddressModeMirror)
{
    RunAddressTest("cs_addr_mirror", D3D11_TEXTURE_ADDRESS_MIRROR,
                   D3D11_TEXTURE_ADDRESS_MIRROR, nullptr);
}

TEST_F(CSGoldenTests, AddressModeBorder)
{
    FLOAT borderColor[4] = { 0.f, 0.f, 1.f, 1.f };
    RunAddressTest("cs_addr_border", D3D11_TEXTURE_ADDRESS_BORDER,
                   D3D11_TEXTURE_ADDRESS_BORDER, borderColor);
}

TEST_F(CSGoldenTests, AddressModeMirrorOnce)
{
    RunAddressTest("cs_addr_mirror_once", D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
                   D3D11_TEXTURE_ADDRESS_MIRROR_ONCE, nullptr);
}
