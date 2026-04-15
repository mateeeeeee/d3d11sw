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

TEST_F(DrawGoldenTests, AlphaBlend64x64)
{
    const UINT W = 64, H = 64;

    // Render target
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

    FLOAT clearColor[4] = {0.f, 0.f, 0.f, 1.f};
    context->ClearRenderTargetView(rtv, clearColor);

    // Two overlapping triangles with alpha=0.5
    float vertices[] = {
        // First triangle (left-ish)
        -0.2f,  0.8f, 0.5f,
         0.4f, -0.8f, 0.5f,
        -0.8f, -0.8f, 0.5f,
        // Second triangle (right-ish, overlapping)
         0.2f,  0.8f, 0.5f,
         0.8f, -0.8f, 0.5f,
        -0.4f, -0.8f, 0.5f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    // Colors: red at 50% alpha, blue at 50% alpha
    float redAlpha[4]  = {1.f, 0.f, 0.f, 0.5f};
    float blueAlpha[4] = {0.f, 0.f, 1.f, 0.5f};

    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = 16;
    cbDesc.Usage     = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA cbInitRed{};
    cbInitRed.pSysMem = redAlpha;
    ID3D11Buffer* cbRed = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInitRed, &cbRed)));

    D3D11_SUBRESOURCE_DATA cbInitBlue{};
    cbInitBlue.pSysMem = blueAlpha;
    ID3D11Buffer* cbBlue = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInitBlue, &cbBlue)));

    // Shaders
    auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_passthrough.o");
    ASSERT_FALSE(vsBytecode.empty());

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* inputLayout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layoutDesc, 1, vsBytecode.data(), vsBytecode.size(), &inputLayout)));

    ID3D11VertexShader* vsShader = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
        vsBytecode.data(), vsBytecode.size(), nullptr, &vsShader)));

    auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_solid_cbuffer.o");
    ASSERT_FALSE(psBytecode.empty());

    ID3D11PixelShader* psShader = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
        psBytecode.data(), psBytecode.size(), nullptr, &psShader)));

    // Blend state: SrcAlpha / InvSrcAlpha
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable           = TRUE;
    blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ID3D11BlendState* blendState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBlendState(&blendDesc, &blendState)));

    // Set state
    context->IASetInputLayout(inputLayout);
    UINT stride = 12;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vsShader, nullptr, 0);
    context->PSSetShader(psShader, nullptr, 0);
    context->OMSetRenderTargets(1, &rtv, nullptr);
    FLOAT blendFactor[4] = {0.f, 0.f, 0.f, 0.f};
    context->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);

    D3D11_VIEWPORT viewport{};
    viewport.Width    = static_cast<FLOAT>(W);
    viewport.Height   = static_cast<FLOAT>(H);
    viewport.MaxDepth = 1.f;
    context->RSSetViewports(1, &viewport);

    // Draw red semi-transparent
    context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cbRed));
    context->Draw(3, 0);

    // Draw blue semi-transparent
    context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cbBlue));
    context->Draw(3, 3);

    // Read back
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

    auto result = CheckGolden("draw_blend_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    // Sanity: overlapping region should have both red and blue components
    bool foundBlend = false;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (pixels[i*4+0] > 0.1f && pixels[i*4+2] > 0.1f)
        {
            foundBlend = true;
            break;
        }
    }
    EXPECT_TRUE(foundBlend) << "Expected blended pixels with both red and blue components";

    staging->Release();
    blendState->Release();
    psShader->Release();
    vsShader->Release();
    inputLayout->Release();
    cbBlue->Release();
    cbRed->Release();
    vb->Release();
    rtv->Release();
    rtTex->Release();
}
