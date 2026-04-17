#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <cstring>
#include <cmath>
#include "golden_test_util.h"

// 1. Multiple PS shaders switched between draws
// 2. Map/Unmap cbuffer updates (DYNAMIC usage)
// 3. B8G8R8X8 texture sampling
// 4. Multiple cbuffer slots (b0, b1, b2)
// 5. PS cbuffer read at high offset (cb2[16])
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

TEST_F(DrawGoldenTests, CombinedPsSwitchMapBgraMultiCbuf64x64)
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

    uint8_t texData[4 * 4 * 4];
    for (int i = 0; i < 4 * 4; ++i)
    {
        texData[i * 4 + 0] = 255; // B
        texData[i * 4 + 1] = 255; // G
        texData[i * 4 + 2] = 255; // R
        texData[i * 4 + 3] = 0;   // X (unused)
    }

    D3D11_TEXTURE2D_DESC tDesc{};
    tDesc.Width            = 4;
    tDesc.Height           = 4;
    tDesc.MipLevels        = 1;
    tDesc.ArraySize        = 1;
    tDesc.Format           = DXGI_FORMAT_B8G8R8X8_UNORM;
    tDesc.SampleDesc.Count = 1;
    tDesc.Usage            = D3D11_USAGE_DEFAULT;
    tDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA tInit{};
    tInit.pSysMem    = texData;
    tInit.SysMemPitch = 4 * 4;
    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&tDesc, &tInit, &tex)));
    ID3D11ShaderResourceView* srv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(tex, nullptr, &srv)));

    D3D11_SAMPLER_DESC smpDesc{};
    smpDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
    smpDesc.AddressU = smpDesc.AddressV = smpDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.MaxLOD   = D3D11_FLOAT32_MAX;
    ID3D11SamplerState* sampler = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateSamplerState(&smpDesc, &sampler)));

    float verts[] = {
        // Left triangle (x, y, z, u, v)
        -1.0f, -1.0f, 0.5f,   0.0f, 1.0f,
         0.0f,  1.0f, 0.5f,   0.5f, 0.0f,
         0.0f, -1.0f, 0.5f,   0.5f, 1.0f,
        // Right triangle
         0.0f, -1.0f, 0.5f,   0.5f, 1.0f,
         0.0f,  1.0f, 0.5f,   0.5f, 0.0f,
         1.0f, -1.0f, 0.5f,   1.0f, 1.0f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(verts);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = verts;
    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    // cb0 (b0): View matrix — identity (64 bytes)
    // cb1 (b1): Projection matrix — identity (64 bytes)
    // cb2 (b2): World (identity) + pad[12] + outputColor at offset 16 — covers #5, #6
    float identity[16] = {};
    identity[0] = identity[5] = identity[10] = identity[15] = 1.f;

    D3D11_BUFFER_DESC cbDescStatic{};
    cbDescStatic.ByteWidth = 64;
    cbDescStatic.Usage     = D3D11_USAGE_DEFAULT;
    cbDescStatic.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA cbInit{};
    cbInit.pSysMem = identity;

    ID3D11Buffer* cb0 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDescStatic, &cbInit, &cb0)));
    ID3D11Buffer* cb1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDescStatic, &cbInit, &cb1)));

    D3D11_BUFFER_DESC cb2Desc{};
    cb2Desc.ByteWidth      = 272;
    cb2Desc.Usage          = D3D11_USAGE_DYNAMIC;
    cb2Desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    cb2Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer* cb2 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cb2Desc, nullptr, &cb2)));

    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        ASSERT_TRUE(SUCCEEDED(context->Map(cb2, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)));
        float* data = (float*)mapped.pData;
        memset(data, 0, 272);
        memcpy(data, identity, 64); // World = identity at offset 0
        // outputColor = RED at offset 16 (float4 index 16 = byte offset 256)
        data[64] = 1.f; data[65] = 0.f; data[66] = 0.f; data[67] = 1.f;
        context->Unmap(cb2, 0);
    }

    auto vsBlob = ReadBytecode(D3D11SW_SHADER_DIR "/vs_multi_cbuf.o");
    auto psTexBlob = ReadBytecode(D3D11SW_SHADER_DIR "/ps_combined_textured.o");
    auto psSolidBlob = ReadBytecode(D3D11SW_SHADER_DIR "/ps_combined_solid.o");
    ASSERT_FALSE(vsBlob.empty());
    ASSERT_FALSE(psTexBlob.empty());
    ASSERT_FALSE(psSolidBlob.empty());

    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, &vs)));
    ID3D11PixelShader* psTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(psTexBlob.data(), psTexBlob.size(), nullptr, &psTex)));
    ID3D11PixelShader* psSolid = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(psSolidBlob.data(), psSolidBlob.size(), nullptr, &psSolid)));

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* layout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(layoutDesc, 2, vsBlob.data(), vsBlob.size(), &layout)));

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode        = D3D11_FILL_SOLID;
    rsDesc.CullMode        = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = TRUE;
    ID3D11RasterizerState* rsState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rsDesc, &rsState)));

    float clearColor[] = {0, 0, 0, 1};
    context->ClearRenderTargetView(rtv, clearColor);
    context->OMSetRenderTargets(1, &rtv, nullptr);
    context->RSSetState(rsState);

    D3D11_VIEWPORT vp{};
    vp.Width = (float)W; vp.Height = (float)H; vp.MaxDepth = 1.f;
    context->RSSetViewports(1, &vp);

    UINT stride = 20, offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetInputLayout(layout);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(vs, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &cb0);
    context->VSSetConstantBuffers(1, 1, &cb1);
    context->VSSetConstantBuffers(2, 1, &cb2);

    context->PSSetShader(psTex, nullptr, 0);
    context->PSSetConstantBuffers(2, 1, &cb2);
    context->PSSetShaderResources(0, 1, &srv);
    context->PSSetSamplers(0, 1, &sampler);
    context->Draw(3, 0); 

    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        ASSERT_TRUE(SUCCEEDED(context->Map(cb2, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)));
        float* data = (float*)mapped.pData;
        memset(data, 0, 272);
        memcpy(data, identity, 64);
        data[64] = 0.f; data[65] = 0.f; data[66] = 1.f; data[67] = 1.f; // BLUE
        context->Unmap(cb2, 0);
    }

    context->PSSetShader(psSolid, nullptr, 0);
    context->Draw(3, 3); 

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
        const uint8_t* row = (const uint8_t*)mapped.pData + y * mapped.RowPitch;
        for (UINT x = 0; x < W; ++x)
        {
            pixels[(y * W + x) * 4 + 0] = row[x * 4 + 0] / 255.f;
            pixels[(y * W + x) * 4 + 1] = row[x * 4 + 1] / 255.f;
            pixels[(y * W + x) * 4 + 2] = row[x * 4 + 2] / 255.f;
            pixels[(y * W + x) * 4 + 3] = row[x * 4 + 3] / 255.f;
        }
    }
    context->Unmap(staging, 0);

    int leftRed = 0;
    for (UINT y = 0; y < H; ++y)
    {
        for (UINT x = 0; x < W / 2; ++x)
        {
            UINT i = y * W + x;
            float r = pixels[i * 4 + 0];
            float g = pixels[i * 4 + 1];
            float b = pixels[i * 4 + 2];
            if (r > 0.9f && g < 0.1f && b < 0.1f) leftRed++;
        }
    }

    int rightBlue = 0;
    for (UINT y = 0; y < H; ++y)
    {
        for (UINT x = W / 2; x < W; ++x)
        {
            UINT i = y * W + x;
            float r = pixels[i * 4 + 0];
            float g = pixels[i * 4 + 1];
            float b = pixels[i * 4 + 2];
            if (r < 0.1f && g < 0.1f && b > 0.9f) rightBlue++;
        }
    }

    EXPECT_GT(leftRed, 200)
        << "Left triangle should be RED (B8G8R8X8 white tex * outputColor from cb2[16])";
    EXPECT_GT(rightBlue, 200)
        << "Right triangle should be BLUE (solid color from cb2[16] after Map/Unmap + PS switch)";

    auto result = CheckGolden("draw_combined_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    staging->Release();
    rsState->Release();
    sampler->Release();
    srv->Release();
    tex->Release();
    psSolid->Release();
    psTex->Release();
    vs->Release();
    layout->Release();
    cb2->Release();
    cb1->Release();
    cb0->Release();
    vb->Release();
    rtv->Release();
    rtTex->Release();
}
