#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <cstring>
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

static void MakeIdentity(float* m)
{
    memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

static void Transpose(const float* in, float* out)
{
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out[c * 4 + r] = in[r * 4 + c];
}

TEST_F(DrawGoldenTests, MultiCbufTransform64x64)
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

    // Simple triangle in NDC: position + texcoord
    // Vertex: x, y, z, u, v
    float verts[] = {
        -0.5f, -0.5f, 0.5f,   0.0f, 1.0f,
         0.5f, -0.5f, 0.5f,   1.0f, 1.0f,
         0.0f,  0.5f, 0.5f,   0.5f, 0.0f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(verts);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = verts;
    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    // Three cbuffers matching Tutorial07 layout
    // cb0 (b0): View matrix (identity)
    // cb1 (b1): Projection matrix (identity)
    // cb2 (b2): World matrix (identity) + vMeshColor (white)
    float identity[16];
    MakeIdentity(identity);
    float identityT[16];
    Transpose(identity, identityT);

    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.Usage     = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    // cb0: View (64 bytes)
    cbDesc.ByteWidth = 64;
    D3D11_SUBRESOURCE_DATA cbInit0{};
    cbInit0.pSysMem = identityT;
    ID3D11Buffer* cb0 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInit0, &cb0)));

    // cb1: Projection (64 bytes)
    cbDesc.ByteWidth = 64;
    D3D11_SUBRESOURCE_DATA cbInit1{};
    cbInit1.pSysMem = identityT;
    ID3D11Buffer* cb1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInit1, &cb1)));

    // cb2: World (64 bytes) + vMeshColor (16 bytes) = 80 bytes
    float cb2Data[20] = {};
    memcpy(cb2Data, identityT, 64);
    cb2Data[16] = 1.f; cb2Data[17] = 0.f; cb2Data[18] = 0.f; cb2Data[19] = 1.f; // red
    cbDesc.ByteWidth = 80;
    D3D11_SUBRESOURCE_DATA cbInit2{};
    cbInit2.pSysMem = cb2Data;
    ID3D11Buffer* cb2 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInit2, &cb2)));

    // 1x1 white texture
    uint8_t white[] = {255, 255, 255, 255};
    D3D11_TEXTURE2D_DESC tDesc{};
    tDesc.Width = tDesc.Height = tDesc.MipLevels = tDesc.ArraySize = 1;
    tDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    tDesc.SampleDesc.Count = 1;
    tDesc.Usage            = D3D11_USAGE_DEFAULT;
    tDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA tInit{};
    tInit.pSysMem    = white;
    tInit.SysMemPitch = 4;
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

    // Shaders
    auto vsBlob = ReadBytecode(D3D11SW_SHADER_DIR "/vs_multi_cbuf.o");
    auto psBlob = ReadBytecode(D3D11SW_SHADER_DIR "/ps_multi_cbuf.o");
    ASSERT_FALSE(vsBlob.empty());
    ASSERT_FALSE(psBlob.empty());

    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, &vs)));
    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, &ps)));

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* layout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(layoutDesc, 2, vsBlob.data(), vsBlob.size(), &layout)));

    // Render
    float clearColor[] = {0, 0, 0, 1};
    context->ClearRenderTargetView(rtv, clearColor);
    context->OMSetRenderTargets(1, &rtv, nullptr);

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

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = TRUE;
    ID3D11RasterizerState* rsState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&rsDesc, &rsState)));
    context->RSSetState(rsState);

    context->PSSetShader(ps, nullptr, 0);
    context->PSSetConstantBuffers(2, 1, &cb2);
    context->PSSetShaderResources(0, 1, &srv);
    context->PSSetSamplers(0, 1, &sampler);

    context->Draw(3, 0);

    // Readback
    D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, rtTex);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    // Convert to float
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

    // With identity matrices, the triangle should be visible (red on black).
    // Check that SOME pixels are non-black (i.e., the triangle rendered).
    int nonBlack = 0;
    int redPixels = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        float r = pixels[i * 4 + 0];
        float g = pixels[i * 4 + 1];
        float b = pixels[i * 4 + 2];
        if (r > 0.01f || g > 0.01f || b > 0.01f)
        {
            if (nonBlack < 5)
            {
                fprintf(stderr, "pixel(%u,%u) = (%.3f, %.3f, %.3f)\n", i % W, i / W, r, g, b);
            }
            nonBlack++;
        }
        if (r > 0.9f && g < 0.1f && b < 0.1f) redPixels++;
    }

    fprintf(stderr, "nonBlack=%d redPixels=%d\n", nonBlack, redPixels);

    EXPECT_GT(nonBlack, 100) << "Triangle should be visible with identity matrices";
    EXPECT_GT(redPixels, 100) << "Triangle should be red (vMeshColor = red, white texture)";

    // Cleanup
    rsState->Release();
    sampler->Release();
    srv->Release();
    tex->Release();
    staging->Release();
    ps->Release();
    vs->Release();
    layout->Release();
    cb2->Release();
    cb1->Release();
    cb0->Release();
    vb->Release();
    rtv->Release();
    rtTex->Release();
}
