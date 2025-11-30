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

TEST_F(DrawGoldenTests, MRT64x64)
{
    const UINT W = 64, H = 64;

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width            = W;
    texDesc.Height           = H;
    texDesc.MipLevels        = 1;
    texDesc.ArraySize        = 1;
    texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage            = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags        = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* rtTex0 = nullptr;
    ID3D11Texture2D* rtTex1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &rtTex0)));
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &rtTex1)));

    ID3D11RenderTargetView* rtv0 = nullptr;
    ID3D11RenderTargetView* rtv1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex0, nullptr, &rtv0)));
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex1, nullptr, &rtv1)));

    FLOAT clearColor[4] = {0.f, 0.f, 0.f, 1.f};
    context->ClearRenderTargetView(rtv0, clearColor);
    context->ClearRenderTargetView(rtv1, clearColor);

    float vertices[] = {
         0.0f,  0.8f, 0.5f,
        -0.8f, -0.8f, 0.5f,
         0.8f, -0.8f, 0.5f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_passthrough.o");
    ASSERT_FALSE(vsBytecode.empty());

    ID3D11InputLayout* inputLayout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layoutDesc, 1, vsBytecode.data(), vsBytecode.size(), &inputLayout)));

    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
        vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));

    auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_mrt2.o");
    ASSERT_FALSE(psBytecode.empty()) << "ps_mrt2.o not found — run compile.sh";

    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
        psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    context->IASetInputLayout(inputLayout);
    UINT stride = 12;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);

    ID3D11RenderTargetView* rtvs[2] = { rtv0, rtv1 };
    context->OMSetRenderTargets(2, rtvs, nullptr);

    D3D11_VIEWPORT viewport{};
    viewport.Width    = static_cast<FLOAT>(W);
    viewport.Height   = static_cast<FLOAT>(H);
    viewport.MaxDepth = 1.f;
    context->RSSetViewports(1, &viewport);

    context->Draw(3, 0);

    D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    auto readBack = [&](ID3D11Texture2D* src) -> std::vector<float>
    {
        ID3D11Texture2D* staging = nullptr;
        device->CreateTexture2D(&stagingDesc, nullptr, &staging);
        context->CopyResource(staging, src);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped);

        std::vector<float> pixels(W * H * 4);
        for (UINT y = 0; y < H; ++y)
        {
            const UINT8* row = static_cast<const UINT8*>(mapped.pData) + y * mapped.RowPitch;
            for (UINT x = 0; x < W; ++x)
            {
                UINT idx = (y * W + x) * 4;
                pixels[idx + 0] = row[x * 4 + 0] / 255.f;
                pixels[idx + 1] = row[x * 4 + 1] / 255.f;
                pixels[idx + 2] = row[x * 4 + 2] / 255.f;
                pixels[idx + 3] = row[x * 4 + 3] / 255.f;
            }
        }
        context->Unmap(staging, 0);
        staging->Release();
        return pixels;
    };

    auto pixels0 = readBack(rtTex0);
    auto pixels1 = readBack(rtTex1);

    auto result0 = CheckGolden("draw_mrt_rt0_64x64", pixels0.data(), W, H, 0.0f);
    EXPECT_TRUE(result0.passed) << result0.message;

    auto result1 = CheckGolden("draw_mrt_rt1_64x64", pixels1.data(), W, H, 0.0f);
    EXPECT_TRUE(result1.passed) << result1.message;

    int redPx = 0, bluePx = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (pixels0[i * 4 + 0] > 0.9f && pixels0[i * 4 + 2] < 0.1f) { ++redPx; }
        if (pixels1[i * 4 + 2] > 0.9f && pixels1[i * 4 + 0] < 0.1f) { ++bluePx; }
    }
    EXPECT_GT(redPx, 100) << "RT0 should have red pixels from oC[0]";
    EXPECT_GT(bluePx, 100) << "RT1 should have blue pixels from oC[1]";

    ps->Release();
    vs->Release();
    inputLayout->Release();
    vb->Release();
    rtv1->Release();
    rtv0->Release();
    rtTex1->Release();
    rtTex0->Release();
}
