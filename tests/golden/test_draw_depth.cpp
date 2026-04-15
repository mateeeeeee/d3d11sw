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

TEST_F(DrawGoldenTests, DepthOcclusion64x64)
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

    // Depth buffer
    D3D11_TEXTURE2D_DESC dsTexDesc{};
    dsTexDesc.Width            = W;
    dsTexDesc.Height           = H;
    dsTexDesc.MipLevels        = 1;
    dsTexDesc.ArraySize        = 1;
    dsTexDesc.Format           = DXGI_FORMAT_D32_FLOAT;
    dsTexDesc.SampleDesc.Count = 1;
    dsTexDesc.Usage            = D3D11_USAGE_DEFAULT;
    dsTexDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* dsTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&dsTexDesc, nullptr, &dsTex)));

    ID3D11DepthStencilView* dsv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilView(dsTex, nullptr, &dsv)));

    FLOAT clearColor[4] = {0.f, 0.f, 0.f, 1.f};
    context->ClearRenderTargetView(rtv, clearColor);
    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Two overlapping triangles:
    // Red triangle at z=0.7 (far), Green triangle at z=0.3 (near)
    // Both cover roughly the center area
    float vertices[] = {
        // Red triangle (far, z=0.7)
         0.0f,  0.7f, 0.7f,
         0.7f, -0.7f, 0.7f,
        -0.7f, -0.7f, 0.7f,
        // Green triangle (near, z=0.3)
         0.0f,  0.5f, 0.3f,
         0.5f, -0.3f, 0.3f,
        -0.5f, -0.3f, 0.3f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    // Constant buffer for PS color
    float redColor[4]   = {1.f, 0.f, 0.f, 1.f};
    float greenColor[4] = {0.f, 1.f, 0.f, 1.f};

    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth      = 16;
    cbDesc.Usage           = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags       = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA cbInitRed{};
    cbInitRed.pSysMem = redColor;
    ID3D11Buffer* cbRed = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInitRed, &cbRed)));

    D3D11_SUBRESOURCE_DATA cbInitGreen{};
    cbInitGreen.pSysMem = greenColor;
    ID3D11Buffer* cbGreen = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInitGreen, &cbGreen)));

    // Input layout
    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_passthrough.o");
    ASSERT_FALSE(vsBytecode.empty()) << "VS shader not found — run compile.sh";

    ID3D11InputLayout* inputLayout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layoutDesc, 1, vsBytecode.data(), vsBytecode.size(), &inputLayout)));

    ID3D11VertexShader* vsShader = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
        vsBytecode.data(), vsBytecode.size(), nullptr, &vsShader)));

    auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_solid_cbuffer.o");
    ASSERT_FALSE(psBytecode.empty()) << "PS shader not found — run compile.sh";

    ID3D11PixelShader* psShader = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
        psBytecode.data(), psBytecode.size(), nullptr, &psShader)));

    // Depth-stencil state
    D3D11_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthEnable    = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc      = D3D11_COMPARISON_LESS;
    ID3D11DepthStencilState* dsState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilState(&dsDesc, &dsState)));

    // Set common state
    context->IASetInputLayout(inputLayout);
    UINT stride = 12;
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vsShader, nullptr, 0);
    context->PSSetShader(psShader, nullptr, 0);
    context->OMSetRenderTargets(1, &rtv, dsv);
    context->OMSetDepthStencilState(dsState, 0);

    D3D11_VIEWPORT viewport{};
    viewport.Width    = static_cast<FLOAT>(W);
    viewport.Height   = static_cast<FLOAT>(H);
    viewport.MaxDepth = 1.f;
    context->RSSetViewports(1, &viewport);

    // Draw red triangle (far) first
    context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cbRed));
    context->Draw(3, 0);

    // Draw green triangle (near) — should overwrite where it overlaps
    context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cbGreen));
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

    auto result = CheckGolden("draw_depth_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    // Sanity: must have both red and green pixels
    int redPx = 0, greenPx = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (pixels[i*4+0] > 0.9f && pixels[i*4+1] < 0.1f) { ++redPx; }
        if (pixels[i*4+1] > 0.9f && pixels[i*4+0] < 0.1f) { ++greenPx; }
    }
    EXPECT_GT(redPx, 20) << "Expected some red pixels from the far triangle";
    EXPECT_GT(greenPx, 50) << "Expected green pixels from the near triangle";

    staging->Release();
    dsState->Release();
    psShader->Release();
    vsShader->Release();
    inputLayout->Release();
    cbGreen->Release();
    cbRed->Release();
    vb->Release();
    dsv->Release();
    dsTex->Release();
    rtv->Release();
    rtTex->Release();
}
