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

TEST_F(DrawGoldenTests, DepthClipDisabled64x64)
{
    const UINT W = 64, H = 64;

    D3D11_TEXTURE2D_DESC rtDesc{};
    rtDesc.Width            = W;
    rtDesc.Height           = H;
    rtDesc.MipLevels        = 1;
    rtDesc.ArraySize        = 1;
    rtDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtDesc.SampleDesc.Count = 1;
    rtDesc.Usage            = D3D11_USAGE_DEFAULT;
    rtDesc.BindFlags        = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* rtTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&rtDesc, nullptr, &rtTex)));
    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex, nullptr, &rtv)));

    FLOAT clearColor[4] = { 0.f, 0.f, 0.f, 1.f };
    context->ClearRenderTargetView(rtv, clearColor);

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode        = D3D11_FILL_SOLID;
    rsDesc.CullMode        = D3D11_CULL_BACK;
    rsDesc.DepthClipEnable = FALSE;

    ID3D11RasterizerState* rs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rsDesc, &rs)));
    context->RSSetState(rs);

    float triVerts[] = {
        -0.5f, -0.5f, 2.0f, 1.f,
         0.0f,  0.5f, 2.0f, 1.f,
         0.5f, -0.5f, 2.0f, 1.f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(triVerts);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = triVerts;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_clip_test.o");
    ASSERT_FALSE(vsBytecode.empty());
    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3D11InputLayout* layout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(layoutDesc, 1, vsBytecode.data(), vsBytecode.size(), &layout)));

    auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_solid_red.o");
    ASSERT_FALSE(psBytecode.empty());
    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    D3D11_VIEWPORT viewport{};
    viewport.Width    = static_cast<FLOAT>(W);
    viewport.Height   = static_cast<FLOAT>(H);
    viewport.MaxDepth = 1.f;
    context->RSSetViewports(1, &viewport);

    context->IASetInputLayout(layout);
    UINT stride = 16, offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->OMSetRenderTargets(1, &rtv, nullptr);

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

    auto result = CheckGolden("draw_depth_clip_disabled_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    int redPx = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (pixels[i * 4 + 0] > 0.9f && pixels[i * 4 + 1] < 0.1f)
        {
            ++redPx;
        }
    }
    EXPECT_GT(redPx, 200) << "Triangle at z=2 should render with DepthClipEnable=FALSE";

    staging->Release();
    ps->Release();
    vs->Release();
    layout->Release();
    vb->Release();
    rs->Release();
    rtv->Release();
    rtTex->Release();
}
