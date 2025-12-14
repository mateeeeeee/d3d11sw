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

    void RunDepthFuncTest(const char* goldenName, D3D11_COMPARISON_FUNC func,
                          float firstZ, float secondZ,
                          float firstColor[4], float secondColor[4],
                          int expectFirst, int expectSecond,
                          float clearDepth = 1.0f)
    {
        const UINT W = 64, H = 64;

        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = W; texDesc.Height = H;
        texDesc.MipLevels = 1; texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

        ID3D11Texture2D* rtTex = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &rtTex)));
        ID3D11RenderTargetView* rtv = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex, nullptr, &rtv)));

        D3D11_TEXTURE2D_DESC dsTexDesc{};
        dsTexDesc.Width = W; dsTexDesc.Height = H;
        dsTexDesc.MipLevels = 1; dsTexDesc.ArraySize = 1;
        dsTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsTexDesc.SampleDesc.Count = 1;
        dsTexDesc.Usage = D3D11_USAGE_DEFAULT;
        dsTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ID3D11Texture2D* dsTex = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&dsTexDesc, nullptr, &dsTex)));
        ID3D11DepthStencilView* dsv = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilView(dsTex, nullptr, &dsv)));

        FLOAT clearColor[4] = {0.f, 0.f, 0.f, 1.f};
        context->ClearRenderTargetView(rtv, clearColor);
        context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, clearDepth, 0);

        float vertices[] = {
            // First triangle (large)
            0.0f,  0.8f, firstZ,
           -0.8f, -0.8f, firstZ,
            0.8f, -0.8f, firstZ,
            // Second triangle (smaller, centered)
            0.0f,  0.5f, secondZ,
           -0.5f, -0.3f, secondZ,
            0.5f, -0.3f, secondZ,
        };

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = sizeof(vertices);
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vbInit{};
        vbInit.pSysMem = vertices;
        ID3D11Buffer* vb = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

        D3D11_BUFFER_DESC cbDesc{};
        cbDesc.ByteWidth = 16;
        cbDesc.Usage = D3D11_USAGE_DEFAULT;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        D3D11_SUBRESOURCE_DATA cb1Init{};
        cb1Init.pSysMem = firstColor;
        ID3D11Buffer* cb1 = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cb1Init, &cb1)));

        D3D11_SUBRESOURCE_DATA cb2Init{};
        cb2Init.pSysMem = secondColor;
        ID3D11Buffer* cb2 = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cb2Init, &cb2)));

        auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_passthrough.o");
        ASSERT_FALSE(vsBytecode.empty());
        auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_solid_cbuffer.o");
        ASSERT_FALSE(psBytecode.empty());

        D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        ID3D11InputLayout* il = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(layoutDesc, 1, vsBytecode.data(), vsBytecode.size(), &il)));
        ID3D11VertexShader* vs = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));
        ID3D11PixelShader* ps = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(psBytecode.data(), psBytecode.size(), nullptr, &ps)));

        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = func;
        ID3D11DepthStencilState* dsState = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilState(&dsDesc, &dsState)));

        context->IASetInputLayout(il);
        UINT stride = 12, offset = 0;
        context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride, &offset);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(vs, nullptr, 0);
        context->PSSetShader(ps, nullptr, 0);
        context->OMSetRenderTargets(1, &rtv, dsv);
        context->OMSetDepthStencilState(dsState, 0);

        D3D11_VIEWPORT viewport{};
        viewport.Width = static_cast<FLOAT>(W);
        viewport.Height = static_cast<FLOAT>(H);
        viewport.MaxDepth = 1.f;
        context->RSSetViewports(1, &viewport);

        context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cb1));
        context->Draw(3, 0);

        context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cb2));
        context->Draw(3, 3);

        D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.BindFlags = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ID3D11Texture2D* staging = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
        context->CopyResource(staging, rtTex);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

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

        auto result = CheckGolden(goldenName, pixels.data(), W, H, 0.0f);
        EXPECT_TRUE(result.passed) << result.message;

        if (expectFirst >= 0 || expectSecond >= 0)
        {
            int c1 = 0, c2 = 0;
            for (UINT i = 0; i < W * H; ++i)
            {
                if (pixels[i*4+0] > 0.9f && pixels[i*4+1] < 0.1f && pixels[i*4+2] < 0.1f) { ++c1; }
                if (pixels[i*4+1] > 0.9f && pixels[i*4+0] < 0.1f && pixels[i*4+2] < 0.1f) { ++c2; }
            }
            if (expectFirst > 0) { EXPECT_GT(c1, expectFirst); }
            if (expectSecond > 0) { EXPECT_GT(c2, expectSecond); }
            if (expectFirst == 0) { EXPECT_EQ(c1, 0); }
            if (expectSecond == 0) { EXPECT_EQ(c2, 0); }
        }

        staging->Release();
        dsState->Release();
        ps->Release(); vs->Release(); il->Release();
        cb2->Release(); cb1->Release(); vb->Release();
        dsv->Release(); dsTex->Release(); rtv->Release(); rtTex->Release();
    }
};

TEST_F(DrawGoldenTests, DepthGreater)
{
    float red[4]   = {1.f, 0.f, 0.f, 1.f};
    float green[4] = {0.f, 1.f, 0.f, 1.f};
    // Clear depth to 0.0: both z=0.3 and z=0.7 pass GREATER.
    // Red draws at z=0.3, then green at z=0.7 overwrites (0.7 > 0.3).
    // Small triangle (green) covers center, large triangle (red) surrounds it.
    RunDepthFuncTest("draw_depth_greater_64x64", D3D11_COMPARISON_GREATER,
                     0.3f, 0.7f, red, green, 20, 50, 0.0f);
}

TEST_F(DrawGoldenTests, DepthEqual)
{
    float red[4]   = {1.f, 0.f, 0.f, 1.f};
    float green[4] = {0.f, 1.f, 0.f, 1.f};
    // Clear depth to 0.5: both draws at z=0.5 pass EQUAL (0.5 == 0.5).
    // Green overwrites red. Result: green triangle on black.
    RunDepthFuncTest("draw_depth_equal_64x64", D3D11_COMPARISON_EQUAL,
                     0.5f, 0.5f, red, green, -1, 50, 0.5f);
}

TEST_F(DrawGoldenTests, DepthAlways)
{
    float red[4]   = {1.f, 0.f, 0.f, 1.f};
    float green[4] = {0.f, 1.f, 0.f, 1.f};
    RunDepthFuncTest("draw_depth_always_64x64", D3D11_COMPARISON_ALWAYS,
                     0.3f, 0.7f, red, green, -1, 50);
}

TEST_F(DrawGoldenTests, DepthNever)
{
    float red[4]   = {1.f, 0.f, 0.f, 1.f};
    float green[4] = {0.f, 1.f, 0.f, 1.f};
    RunDepthFuncTest("draw_depth_never_64x64", D3D11_COMPARISON_NEVER,
                     0.3f, 0.7f, red, green, 0, 0);
}
