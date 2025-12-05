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

TEST_F(DrawGoldenTests, WriteMaskRedOnly64x64)
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

    ID3D11Texture2D* rtTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &rtTex)));

    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex, nullptr, &rtv)));

    // Clear to mid-gray so we can check which channels get written
    FLOAT clearColor[4] = {0.5f, 0.5f, 0.5f, 0.5f};
    context->ClearRenderTargetView(rtv, clearColor);

    // Full-screen triangle (white)
    float vertices[] = {
        -1.f,  3.f, 0.5f,
        -1.f, -1.f, 0.5f,
         3.f, -1.f, 0.5f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    // Color cbuffer: white
    float white[4] = {1.f, 1.f, 1.f, 1.f};
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = 16;
    cbDesc.Usage     = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA cbInit{};
    cbInit.pSysMem = white;
    ID3D11Buffer* cb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInit, &cb)));

    auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_passthrough.o");
    ASSERT_FALSE(vsBytecode.empty());

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* inputLayout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layoutDesc, 1, vsBytecode.data(), vsBytecode.size(), &inputLayout)));

    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
        vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));

    auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_solid_cbuffer.o");
    ASSERT_FALSE(psBytecode.empty());

    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
        psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    // Blend state: write mask RED only
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable           = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED;

    ID3D11BlendState* blendState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBlendState(&blendDesc, &blendState)));

    context->IASetInputLayout(inputLayout);
    UINT stride = 12, offset = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cb));
    context->OMSetRenderTargets(1, &rtv, nullptr);
    FLOAT blendFactor[4] = {0.f, 0.f, 0.f, 0.f};
    context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);

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

    auto result = CheckGolden("draw_write_mask_red_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    // Sanity: R should be 1.0 (written), G/B/A should be ~0.5 (preserved from clear)
    UINT centerIdx = (H / 2 * W + W / 2) * 4;
    EXPECT_GT(pixels[centerIdx + 0], 0.9f)  << "R should have been written to 1.0";
    EXPECT_NEAR(pixels[centerIdx + 1], 0.5f, 0.02f) << "G should be preserved at 0.5";
    EXPECT_NEAR(pixels[centerIdx + 2], 0.5f, 0.02f) << "B should be preserved at 0.5";

    staging->Release();
    blendState->Release();
    ps->Release();
    vs->Release();
    inputLayout->Release();
    cb->Release();
    vb->Release();
    rtv->Release();
    rtTex->Release();
}

TEST_F(DrawGoldenTests, WriteMaskGB64x64)
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

    ID3D11Texture2D* rtTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &rtTex)));

    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex, nullptr, &rtv)));

    FLOAT clearColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    context->ClearRenderTargetView(rtv, clearColor);

    // Full-screen triangle (green+blue)
    float vertices[] = {
        -1.f,  3.f, 0.5f,
        -1.f, -1.f, 0.5f,
         3.f, -1.f, 0.5f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    float cyan[4] = {0.f, 1.f, 1.f, 0.f};
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = 16;
    cbDesc.Usage     = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA cbInit{};
    cbInit.pSysMem = cyan;
    ID3D11Buffer* cb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInit, &cb)));

    auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_passthrough.o");
    ASSERT_FALSE(vsBytecode.empty());

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* inputLayout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layoutDesc, 1, vsBytecode.data(), vsBytecode.size(), &inputLayout)));

    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
        vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));

    auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_solid_cbuffer.o");
    ASSERT_FALSE(psBytecode.empty());

    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
        psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable           = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;

    ID3D11BlendState* blendState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBlendState(&blendDesc, &blendState)));

    context->IASetInputLayout(inputLayout);
    UINT stride = 12, offset = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cb));
    context->OMSetRenderTargets(1, &rtv, nullptr);
    FLOAT blendFactor[4] = {0.f, 0.f, 0.f, 0.f};
    context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);

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

    auto result = CheckGolden("draw_write_mask_gb_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    // Sanity: R=1.0 (preserved), G=1.0 (written), B=1.0 (written) -> white center
    UINT centerIdx = (H / 2 * W + W / 2) * 4;
    EXPECT_GT(pixels[centerIdx + 0], 0.9f) << "R should be preserved at 1.0";
    EXPECT_GT(pixels[centerIdx + 1], 0.9f) << "G should have been written to 1.0";
    EXPECT_GT(pixels[centerIdx + 2], 0.9f) << "B should have been written to 1.0";

    staging->Release();
    blendState->Release();
    ps->Release();
    vs->Release();
    inputLayout->Release();
    cb->Release();
    vb->Release();
    rtv->Release();
    rtTex->Release();
}
