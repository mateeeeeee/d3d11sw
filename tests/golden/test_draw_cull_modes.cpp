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

    struct DrawSetup
    {
        ID3D11Texture2D*        rtTex       = nullptr;
        ID3D11RenderTargetView* rtv         = nullptr;
        ID3D11Buffer*           vb          = nullptr;
        ID3D11InputLayout*      inputLayout = nullptr;
        ID3D11VertexShader*     vs          = nullptr;
        ID3D11PixelShader*      ps          = nullptr;
        ID3D11Texture2D*        staging     = nullptr;

        void Release()
        {
            if (staging)     { staging->Release(); }
            if (ps)          { ps->Release(); }
            if (vs)          { vs->Release(); }
            if (inputLayout) { inputLayout->Release(); }
            if (vb)          { vb->Release(); }
            if (rtv)         { rtv->Release(); }
            if (rtTex)       { rtTex->Release(); }
        }
    };

    DrawSetup SetupTriangleDraw(UINT W, UINT H)
    {
        DrawSetup s;

        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width            = W;
        texDesc.Height           = H;
        texDesc.MipLevels        = 1;
        texDesc.ArraySize        = 1;
        texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage            = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags        = D3D11_BIND_RENDER_TARGET;
        device->CreateTexture2D(&texDesc, nullptr, &s.rtTex);
        device->CreateRenderTargetView(s.rtTex, nullptr, &s.rtv);

        FLOAT clearColor[4] = {0.f, 0.f, 0.f, 1.f};
        context->ClearRenderTargetView(s.rtv, clearColor);

        // CW triangle covering roughly the center
        float vertices[] = {
             0.0f,  0.8f, 0.5f,
             0.8f, -0.8f, 0.5f,
            -0.8f, -0.8f, 0.5f,
        };

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = sizeof(vertices);
        vbDesc.Usage     = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vbInit{};
        vbInit.pSysMem = vertices;
        device->CreateBuffer(&vbDesc, &vbInit, &s.vb);

        D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_passthrough.o");
        device->CreateInputLayout(
            layoutDesc, 1, vsBytecode.data(), vsBytecode.size(), &s.inputLayout);
        device->CreateVertexShader(
            vsBytecode.data(), vsBytecode.size(), nullptr, &s.vs);

        auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_solid_red.o");
        device->CreatePixelShader(
            psBytecode.data(), psBytecode.size(), nullptr, &s.ps);

        context->IASetInputLayout(s.inputLayout);
        UINT stride = 12, offset = 0;
        context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&s.vb), &stride, &offset);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(s.vs, nullptr, 0);
        context->PSSetShader(s.ps, nullptr, 0);
        context->OMSetRenderTargets(1, &s.rtv, nullptr);

        D3D11_VIEWPORT viewport{};
        viewport.Width    = static_cast<FLOAT>(W);
        viewport.Height   = static_cast<FLOAT>(H);
        viewport.MaxDepth = 1.f;
        context->RSSetViewports(1, &viewport);

        D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
        stagingDesc.Usage          = D3D11_USAGE_STAGING;
        stagingDesc.BindFlags      = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        device->CreateTexture2D(&stagingDesc, nullptr, &s.staging);

        return s;
    }

    std::vector<float> ReadBack(DrawSetup& s, UINT W, UINT H)
    {
        context->CopyResource(s.staging, s.rtTex);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        context->Map(s.staging, 0, D3D11_MAP_READ, 0, &mapped);

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
        context->Unmap(s.staging, 0);
        return pixels;
    }
};

TEST_F(DrawGoldenTests, CullBack64x64)
{
    const UINT W = 64, H = 64;
    auto s = SetupTriangleDraw(W, H);
    ASSERT_NE(s.vs, nullptr);

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode        = D3D11_FILL_SOLID;
    rsDesc.CullMode        = D3D11_CULL_BACK;
    rsDesc.DepthClipEnable = TRUE;

    ID3D11RasterizerState* rs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rsDesc, &rs)));
    context->RSSetState(rs);

    context->Draw(3, 0);
    auto pixels = ReadBack(s, W, H);

    auto result = CheckGolden("draw_cull_back_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    // CW triangle with CullBack + default FrontCounterClockwise=FALSE means
    // the triangle IS front-facing, so it should be drawn
    int redPx = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (pixels[i*4+0] > 0.9f && pixels[i*4+1] < 0.1f) { ++redPx; }
    }
    EXPECT_GT(redPx, 500) << "CW triangle should be visible with CULL_BACK (FrontCCW=false)";

    rs->Release();
    s.Release();
}

TEST_F(DrawGoldenTests, CullFront64x64)
{
    const UINT W = 64, H = 64;
    auto s = SetupTriangleDraw(W, H);
    ASSERT_NE(s.vs, nullptr);

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode        = D3D11_FILL_SOLID;
    rsDesc.CullMode        = D3D11_CULL_FRONT;
    rsDesc.DepthClipEnable = TRUE;

    ID3D11RasterizerState* rs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rsDesc, &rs)));
    context->RSSetState(rs);

    context->Draw(3, 0);
    auto pixels = ReadBack(s, W, H);

    auto result = CheckGolden("draw_cull_front_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    // CW triangle with CullFront + FrontCounterClockwise=FALSE => triangle IS front-facing
    // so it should be CULLED
    int redPx = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (pixels[i*4+0] > 0.9f && pixels[i*4+1] < 0.1f) { ++redPx; }
    }
    EXPECT_EQ(redPx, 0) << "CW triangle should be culled with CULL_FRONT (FrontCCW=false)";

    rs->Release();
    s.Release();
}

TEST_F(DrawGoldenTests, CullNone64x64)
{
    const UINT W = 64, H = 64;
    auto s = SetupTriangleDraw(W, H);
    ASSERT_NE(s.vs, nullptr);

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode        = D3D11_FILL_SOLID;
    rsDesc.CullMode        = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = TRUE;

    ID3D11RasterizerState* rs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rsDesc, &rs)));
    context->RSSetState(rs);

    context->Draw(3, 0);
    auto pixels = ReadBack(s, W, H);

    auto result = CheckGolden("draw_cull_none_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    int redPx = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (pixels[i*4+0] > 0.9f && pixels[i*4+1] < 0.1f) { ++redPx; }
    }
    EXPECT_GT(redPx, 500) << "Triangle should always be visible with CULL_NONE";

    rs->Release();
    s.Release();
}

TEST_F(DrawGoldenTests, CullFrontCCW64x64)
{
    const UINT W = 64, H = 64;
    auto s = SetupTriangleDraw(W, H);
    ASSERT_NE(s.vs, nullptr);

    // With FrontCounterClockwise=TRUE, the CW triangle becomes back-facing
    // CullFront should NOT cull it
    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode              = D3D11_FILL_SOLID;
    rsDesc.CullMode              = D3D11_CULL_FRONT;
    rsDesc.FrontCounterClockwise = TRUE;
    rsDesc.DepthClipEnable       = TRUE;

    ID3D11RasterizerState* rs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rsDesc, &rs)));
    context->RSSetState(rs);

    context->Draw(3, 0);
    auto pixels = ReadBack(s, W, H);

    auto result = CheckGolden("draw_cull_front_ccw_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    int redPx = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (pixels[i*4+0] > 0.9f && pixels[i*4+1] < 0.1f) { ++redPx; }
    }
    EXPECT_GT(redPx, 500) << "CW triangle should be visible with CullFront + FrontCCW=true";

    rs->Release();
    s.Release();
}
