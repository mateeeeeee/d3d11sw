#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <cmath>
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

TEST_F(DrawGoldenTests, TexturedCube64x64)
{
    const UINT W = 64, H = 64;
    const UINT TW = 4, TH = 4;

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

    // Clear to cornflower blue + depth 1.0
    FLOAT clearColor[4] = {0.392f, 0.584f, 0.929f, 1.f};
    context->ClearRenderTargetView(rtv, clearColor);
    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // 4x4 checkerboard texture: alternating white and dark grey
    uint8_t texData[TW * TH * 4];
    for (UINT y = 0; y < TH; ++y)
    {
        for (UINT x = 0; x < TW; ++x)
        {
            uint8_t* p = texData + (y * TW + x) * 4;
            uint8_t v = ((x + y) & 1) ? 80 : 255;
            p[0] = v; p[1] = v; p[2] = v; p[3] = 255;
        }
    }

    D3D11_TEXTURE2D_DESC srcTexDesc{};
    srcTexDesc.Width            = TW;
    srcTexDesc.Height           = TH;
    srcTexDesc.MipLevels        = 1;
    srcTexDesc.ArraySize        = 1;
    srcTexDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    srcTexDesc.SampleDesc.Count = 1;
    srcTexDesc.Usage            = D3D11_USAGE_DEFAULT;
    srcTexDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA texInit{};
    texInit.pSysMem     = texData;
    texInit.SysMemPitch = TW * 4;

    ID3D11Texture2D* srcTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&srcTexDesc, &texInit, &srcTex)));

    ID3D11ShaderResourceView* srv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(srcTex, nullptr, &srv)));

    D3D11_SAMPLER_DESC smpDesc{};
    smpDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    smpDesc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.MaxLOD         = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* sampler = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateSamplerState(&smpDesc, &sampler)));

    struct Vertex { float x, y, z, nx, ny, nz, u, v; };
    Vertex vertices[] = {
        // Front face (z = +0.5, normal 0,0,1)
        {-0.5f, -0.5f,  0.5f,  0,0,1,  0.f, 1.f},
        { 0.5f, -0.5f,  0.5f,  0,0,1,  1.f, 1.f},
        { 0.5f,  0.5f,  0.5f,  0,0,1,  1.f, 0.f},
        {-0.5f,  0.5f,  0.5f,  0,0,1,  0.f, 0.f},
        // Back face (z = -0.5, normal 0,0,-1)
        { 0.5f, -0.5f, -0.5f,  0,0,-1,  0.f, 1.f},
        {-0.5f, -0.5f, -0.5f,  0,0,-1,  1.f, 1.f},
        {-0.5f,  0.5f, -0.5f,  0,0,-1,  1.f, 0.f},
        { 0.5f,  0.5f, -0.5f,  0,0,-1,  0.f, 0.f},
        // Right face (x = +0.5, normal 1,0,0)
        { 0.5f, -0.5f,  0.5f,  1,0,0,  0.f, 1.f},
        { 0.5f, -0.5f, -0.5f,  1,0,0,  1.f, 1.f},
        { 0.5f,  0.5f, -0.5f,  1,0,0,  1.f, 0.f},
        { 0.5f,  0.5f,  0.5f,  1,0,0,  0.f, 0.f},
        // Left face (x = -0.5, normal -1,0,0)
        {-0.5f, -0.5f, -0.5f,  -1,0,0,  0.f, 1.f},
        {-0.5f, -0.5f,  0.5f,  -1,0,0,  1.f, 1.f},
        {-0.5f,  0.5f,  0.5f,  -1,0,0,  1.f, 0.f},
        {-0.5f,  0.5f, -0.5f,  -1,0,0,  0.f, 0.f},
        // Top face (y = +0.5, normal 0,1,0)
        {-0.5f,  0.5f,  0.5f,  0,1,0,  0.f, 1.f},
        { 0.5f,  0.5f,  0.5f,  0,1,0,  1.f, 1.f},
        { 0.5f,  0.5f, -0.5f,  0,1,0,  1.f, 0.f},
        {-0.5f,  0.5f, -0.5f,  0,1,0,  0.f, 0.f},
        // Bottom face (y = -0.5, normal 0,-1,0)
        {-0.5f, -0.5f, -0.5f,  0,-1,0,  0.f, 1.f},
        { 0.5f, -0.5f, -0.5f,  0,-1,0,  1.f, 1.f},
        { 0.5f, -0.5f,  0.5f,  0,-1,0,  1.f, 0.f},
        {-0.5f, -0.5f,  0.5f,  0,-1,0,  0.f, 0.f},
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    UINT16 indices[] = {
         0,  2,  1,   0,  3,  2,   // front
         4,  6,  5,   4,  7,  6,   // back
         8, 10,  9,   8, 11, 10,   // right
        12, 14, 13,  12, 15, 14,   // left
        16, 18, 17,  16, 19, 18,   // top
        20, 22, 21,  20, 23, 22,   // bottom
    };

    D3D11_BUFFER_DESC ibDesc{};
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.Usage     = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibInit{};
    ibInit.pSysMem = indices;

    ID3D11Buffer* ib = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&ibDesc, &ibInit, &ib)));

    // Precomputed MVP (column-major for HLSL cbuffer default layout)
    // Camera at (1.2, 1.0, 1.8) looking at origin, 60deg FOV, near=0.1 far=10
    float mvp[16] = {
         1.44115338f,  0.00000000f, -0.96076892f,  0.00000000f,
        -0.40312968f,  1.57220577f, -0.60469453f,  0.00000000f,
        -0.50859476f, -0.42382897f, -0.76289214f,  2.30633844f,
        -0.50350881f, -0.41959068f, -0.75526322f,  2.38327506f,
    };

    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = 64; // 4x4 floats
    cbDesc.Usage     = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA cbInit{};
    cbInit.pSysMem = mvp;

    ID3D11Buffer* cb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInit, &cb)));

    auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_transform_texcoord.o");
    ASSERT_FALSE(vsBytecode.empty()) << "vs_transform_texcoord.o not found — run compile.sh";

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* inputLayout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layoutDesc, 3, vsBytecode.data(), vsBytecode.size(), &inputLayout)));

    ID3D11VertexShader* vsShader = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
        vsBytecode.data(), vsBytecode.size(), nullptr, &vsShader)));

    auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_textured_normal.o");
    ASSERT_FALSE(psBytecode.empty()) << "ps_textured_normal.o not found — run compile.sh";

    ID3D11PixelShader* psShader = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
        psBytecode.data(), psBytecode.size(), nullptr, &psShader)));

    D3D11_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthEnable    = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc      = D3D11_COMPARISON_LESS;
    ID3D11DepthStencilState* dsState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilState(&dsDesc, &dsState)));

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode        = D3D11_FILL_SOLID;
    rsDesc.CullMode        = D3D11_CULL_BACK;
    rsDesc.DepthClipEnable = TRUE;
    ID3D11RasterizerState* rsState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rsDesc, &rsState)));

    context->IASetInputLayout(inputLayout);
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride, &offset);
    context->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vsShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cb));
    context->PSSetShader(psShader, nullptr, 0);
    context->PSSetShaderResources(0, 1, &srv);
    context->PSSetSamplers(0, 1, &sampler);
    context->OMSetRenderTargets(1, &rtv, dsv);
    context->OMSetDepthStencilState(dsState, 0);
    context->RSSetState(rsState);

    D3D11_VIEWPORT viewport{};
    viewport.Width    = static_cast<FLOAT>(W);
    viewport.Height   = static_cast<FLOAT>(H);
    viewport.MaxDepth = 1.f;
    context->RSSetViewports(1, &viewport);

    context->DrawIndexed(36, 0, 0);

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

    auto result = CheckGolden("draw_textured_cube_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    UINT cx = W / 2, cy = H / 2;
    UINT ci = (cy * W + cx) * 4;
    bool centerIsBg = (std::abs(pixels[ci+0] - 0.392f) < 0.02f &&
                       std::abs(pixels[ci+1] - 0.584f) < 0.02f &&
                       std::abs(pixels[ci+2] - 0.929f) < 0.02f);
    EXPECT_FALSE(centerIsBg) << "Center pixel should not be background — cube should be visible";

    UINT cornerIdx = 0;
    bool cornerIsBg = (std::abs(pixels[cornerIdx+0] - 0.392f) < 0.02f &&
                       std::abs(pixels[cornerIdx+1] - 0.584f) < 0.02f &&
                       std::abs(pixels[cornerIdx+2] - 0.929f) < 0.02f);
    EXPECT_TRUE(cornerIsBg) << "Corner pixel should be background color";

    int bluish = 0, reddish = 0, greenish = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        float r = pixels[i*4+0], g = pixels[i*4+1], b = pixels[i*4+2];
        if (b > r + 0.1f && b > g + 0.1f) { ++bluish; }
        if (r > g + 0.1f && r > b + 0.1f) { ++reddish; }
        if (g > r + 0.1f && g > b + 0.1f) { ++greenish; }
    }
    EXPECT_GT(bluish, 20) << "Should have blue-tinted pixels (front face)";
    EXPECT_GT(reddish, 20) << "Should have red-tinted pixels (right face)";
    EXPECT_GT(greenish, 20) << "Should have green-tinted pixels (top face)";

    staging->Release();
    rsState->Release();
    dsState->Release();
    psShader->Release();
    vsShader->Release();
    inputLayout->Release();
    cb->Release();
    ib->Release();
    vb->Release();
    sampler->Release();
    srv->Release();
    srcTex->Release();
    dsv->Release();
    dsTex->Release();
    rtv->Release();
    rtTex->Release();
}
