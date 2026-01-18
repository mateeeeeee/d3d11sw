#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "golden_test_util.h"

struct DrawGoldenTests : ::testing::Test
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

TEST_F(DrawGoldenTests, SampleAutoLod8x8)
{
    const UINT W = 8, H = 8;

    unsigned char mip0[4 * 4 * 4];
    unsigned char mip1[2 * 2 * 4];
    unsigned char mip2[1 * 1 * 4];

    for (UINT i = 0; i < 4 * 4; ++i)
    {
        mip0[i * 4 + 0] = 255; mip0[i * 4 + 1] = 0; mip0[i * 4 + 2] = 0; mip0[i * 4 + 3] = 255;
    }
    for (UINT i = 0; i < 2 * 2; ++i)
    {
        mip1[i * 4 + 0] = 0; mip1[i * 4 + 1] = 255; mip1[i * 4 + 2] = 0; mip1[i * 4 + 3] = 255;
    }
    mip2[0] = 0; mip2[1] = 0; mip2[2] = 255; mip2[3] = 255;

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width            = 4;
    texDesc.Height           = 4;
    texDesc.MipLevels        = 3;
    texDesc.ArraySize        = 1;
    texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage            = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData[3]{};
    initData[0].pSysMem     = mip0;
    initData[0].SysMemPitch = 4 * 4;
    initData[1].pSysMem     = mip1;
    initData[1].SysMemPitch = 2 * 4;
    initData[2].pSysMem     = mip2;
    initData[2].SysMemPitch = 1 * 4;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, initData, &tex)));

    ID3D11ShaderResourceView* srv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(tex, nullptr, &srv)));

    D3D11_SAMPLER_DESC smpDesc{};
    smpDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    smpDesc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.MaxLOD         = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* sampler = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateSamplerState(&smpDesc, &sampler)));

    auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_fullscreen.o");
    auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_sample_auto_lod.o");
    ASSERT_FALSE(vsBytecode.empty()) << "vs_fullscreen.o not found";
    ASSERT_FALSE(psBytecode.empty()) << "ps_sample_auto_lod.o not found";

    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));

    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    D3D11_TEXTURE2D_DESC rtDesc{};
    rtDesc.Width            = W;
    rtDesc.Height           = H;
    rtDesc.MipLevels        = 1;
    rtDesc.ArraySize        = 1;
    rtDesc.Format           = DXGI_FORMAT_R32G32B32A32_FLOAT;
    rtDesc.SampleDesc.Count = 1;
    rtDesc.Usage            = D3D11_USAGE_DEFAULT;
    rtDesc.BindFlags        = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* rtTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&rtDesc, nullptr, &rtTex)));

    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex, nullptr, &rtv)));

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = TRUE;

    ID3D11RasterizerState* rsState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rsDesc, &rsState)));

    D3D11_VIEWPORT vp{};
    vp.Width    = (FLOAT)W;
    vp.Height   = (FLOAT)H;
    vp.MaxDepth = 1.f;

    FLOAT clearColor[4] = {0, 0, 0, 0};
    context->ClearRenderTargetView(rtv, clearColor);
    context->OMSetRenderTargets(1, &rtv, nullptr);
    context->RSSetViewports(1, &vp);
    context->RSSetState(rsState);
    context->VSSetShader(vs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->PSSetShaderResources(0, 1, &srv);
    context->PSSetSamplers(0, 1, &sampler);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(3, 0);

    D3D11_TEXTURE2D_DESC stagingDesc = rtDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, rtTex);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    auto pixels = static_cast<const float*>(mapped.pData);
    auto result = CheckGolden("draw_sample_auto_lod_8x8", pixels, W, H, 0.02f);
    EXPECT_TRUE(result.passed) << result.message;

    context->Unmap(staging, 0);
    staging->Release();
    rsState->Release();
    rtv->Release();
    rtTex->Release();
    ps->Release();
    vs->Release();
    sampler->Release();
    srv->Release();
    tex->Release();
}
