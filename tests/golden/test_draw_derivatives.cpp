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

TEST_F(DrawGoldenTests, Derivatives8x8)
{
    const UINT W = 8, H = 8;

    auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_fullscreen.o");
    auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_derivatives.o");
    ASSERT_FALSE(vsBytecode.empty()) << "vs_fullscreen.o not found — run compile.sh";
    ASSERT_FALSE(psBytecode.empty()) << "ps_derivatives.o not found — run compile.sh";

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

    D3D11_VIEWPORT vp{};
    vp.Width    = (FLOAT)W;
    vp.Height   = (FLOAT)H;
    vp.MaxDepth = 1.f;

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = TRUE;

    ID3D11RasterizerState* rsState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rsDesc, &rsState)));

    FLOAT clearColor[4] = {0, 0, 0, 0};
    context->ClearRenderTargetView(rtv, clearColor);
    context->OMSetRenderTargets(1, &rtv, nullptr);
    context->RSSetViewports(1, &vp);
    context->RSSetState(rsState);
    context->VSSetShader(vs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
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
    auto result = CheckGolden("draw_derivatives_8x8", pixels, W, H, 0.02f);
    EXPECT_TRUE(result.passed) << result.message;

    context->Unmap(staging, 0);
    staging->Release();
    rsState->Release();
    rtv->Release();
    rtTex->Release();
    ps->Release();
    vs->Release();
}
