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

static ID3D11Texture2D* CreateRT(ID3D11Device* dev, UINT W, UINT H, ID3D11RenderTargetView** rtv)
{
    D3D11_TEXTURE2D_DESC td{};
    td.Width = W; td.Height = H; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_RENDER_TARGET;
    ID3D11Texture2D* tex = nullptr;
    dev->CreateTexture2D(&td, nullptr, &tex);
    dev->CreateRenderTargetView(tex, nullptr, rtv);
    return tex;
}

static std::vector<float> Readback(ID3D11Device* dev, ID3D11DeviceContext* ctx, ID3D11Texture2D* tex, UINT W, UINT H)
{
    D3D11_TEXTURE2D_DESC td{};
    tex->GetDesc(&td);
    td.Usage = D3D11_USAGE_STAGING; td.BindFlags = 0; td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* staging = nullptr;
    dev->CreateTexture2D(&td, nullptr, &staging);
    ctx->CopyResource(staging, tex);
    D3D11_MAPPED_SUBRESOURCE m{};
    ctx->Map(staging, 0, D3D11_MAP_READ, 0, &m);
    std::vector<float> px(W * H * 4);
    for (UINT y = 0; y < H; ++y)
    {
        const UINT8* row = static_cast<const UINT8*>(m.pData) + y * m.RowPitch;
        for (UINT x = 0; x < W; ++x)
        {
            UINT i = (y * W + x) * 4;
            px[i+0] = row[x*4+0] / 255.f;
            px[i+1] = row[x*4+1] / 255.f;
            px[i+2] = row[x*4+2] / 255.f;
            px[i+3] = row[x*4+3] / 255.f;
        }
    }
    ctx->Unmap(staging, 0);
    staging->Release();
    return px;
}

static void SetViewport(ID3D11DeviceContext* ctx, UINT W, UINT H)
{
    D3D11_VIEWPORT vp{}; vp.Width = (FLOAT)W; vp.Height = (FLOAT)H; vp.MaxDepth = 1.f;
    ctx->RSSetViewports(1, &vp);
}

TEST_F(DrawGoldenTests, TessTriSimple)
{
    const UINT W = 128, H = 128;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11Texture2D* rtTex = CreateRT(device, W, H, &rtv);
    FLOAT clear[4] = {0,0,0,1};
    context->ClearRenderTargetView(rtv, clear);
    context->OMSetRenderTargets(1, &rtv, nullptr);
    SetViewport(context, W, H);

    float verts[] = {
         0.0f,  0.7f, 0.5f,
         0.7f, -0.7f, 0.5f,
        -0.7f, -0.7f, 0.5f,
    };

    D3D11_BUFFER_DESC bd{}; bd.ByteWidth = sizeof(verts); bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA sd{}; sd.pSysMem = verts;
    ID3D11Buffer* vb = nullptr; device->CreateBuffer(&bd, &sd, &vb);
    UINT stride = 12, offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

    D3D11_INPUT_ELEMENT_DESC il[] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0} };
    auto vsBC = ReadBytecode(D3D11SW_SHADER_DIR "/vs_passthrough.o");
    ASSERT_FALSE(vsBC.empty());
    ID3D11InputLayout* layout = nullptr; device->CreateInputLayout(il, 1, vsBC.data(), vsBC.size(), &layout);
    context->IASetInputLayout(layout);

    ID3D11VertexShader* vs = nullptr; device->CreateVertexShader(vsBC.data(), vsBC.size(), nullptr, &vs);
    context->VSSetShader(vs, nullptr, 0);

    auto hsBC = ReadBytecode(D3D11SW_SHADER_DIR "/hs_simple_tri.o");
    ASSERT_FALSE(hsBC.empty()) << "HS not compiled — run compile.sh";
    ID3D11HullShader* hs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateHullShader(hsBC.data(), hsBC.size(), nullptr, &hs)));
    context->HSSetShader(hs, nullptr, 0);

    auto dsBC = ReadBytecode(D3D11SW_SHADER_DIR "/ds_simple_tri.o");
    ASSERT_FALSE(dsBC.empty()) << "DS not compiled — run compile.sh";
    ID3D11DomainShader* ds = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDomainShader(dsBC.data(), dsBC.size(), nullptr, &ds)));
    context->DSSetShader(ds, nullptr, 0);

    auto psBC = ReadBytecode(D3D11SW_SHADER_DIR "/ps_solid_red.o");
    ID3D11PixelShader* ps = nullptr; device->CreatePixelShader(psBC.data(), psBC.size(), nullptr, &ps);
    context->PSSetShader(ps, nullptr, 0);

    context->Draw(3, 0);

    auto px = Readback(device, context, rtTex, W, H);
    auto result = CheckGolden("draw_tess_tri_simple", px.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    int redPixels = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (px[i*4+0] > 0.9f && px[i*4+1] < 0.1f && px[i*4+2] < 0.1f) { ++redPixels; }
    }
    EXPECT_GT(redPixels, 500) << "Expected tessellated triangle to produce red pixels";

    ps->Release(); ds->Release(); hs->Release(); vs->Release();
    layout->Release(); vb->Release(); rtv->Release(); rtTex->Release();
}

TEST_F(DrawGoldenTests, TessQuadSimple)
{
    const UINT W = 128, H = 128;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11Texture2D* rtTex = CreateRT(device, W, H, &rtv);
    FLOAT clear[4] = {0,0,0,1};
    context->ClearRenderTargetView(rtv, clear);
    context->OMSetRenderTargets(1, &rtv, nullptr);
    SetViewport(context, W, H);

    float verts[] = {
        -0.7f,  0.7f, 0.5f,
         0.7f,  0.7f, 0.5f,
         0.7f, -0.7f, 0.5f,
        -0.7f, -0.7f, 0.5f,
    };

    D3D11_BUFFER_DESC bd{}; bd.ByteWidth = sizeof(verts); bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA sd{}; sd.pSysMem = verts;
    ID3D11Buffer* vb = nullptr; device->CreateBuffer(&bd, &sd, &vb);
    UINT stride = 12, offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

    D3D11_INPUT_ELEMENT_DESC il[] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0} };
    auto vsBC = ReadBytecode(D3D11SW_SHADER_DIR "/vs_passthrough.o");
    ID3D11InputLayout* layout = nullptr; device->CreateInputLayout(il, 1, vsBC.data(), vsBC.size(), &layout);
    context->IASetInputLayout(layout);

    ID3D11VertexShader* vs = nullptr; device->CreateVertexShader(vsBC.data(), vsBC.size(), nullptr, &vs);
    context->VSSetShader(vs, nullptr, 0);

    auto hsBC = ReadBytecode(D3D11SW_SHADER_DIR "/hs_simple_quad.o");
    ASSERT_FALSE(hsBC.empty()) << "HS not compiled — run compile.sh";
    ID3D11HullShader* hs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateHullShader(hsBC.data(), hsBC.size(), nullptr, &hs)));
    context->HSSetShader(hs, nullptr, 0);

    auto dsBC = ReadBytecode(D3D11SW_SHADER_DIR "/ds_simple_quad.o");
    ASSERT_FALSE(dsBC.empty()) << "DS not compiled — run compile.sh";
    ID3D11DomainShader* ds = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDomainShader(dsBC.data(), dsBC.size(), nullptr, &ds)));
    context->DSSetShader(ds, nullptr, 0);

    auto psBC = ReadBytecode(D3D11SW_SHADER_DIR "/ps_solid_red.o");
    ID3D11PixelShader* ps = nullptr; device->CreatePixelShader(psBC.data(), psBC.size(), nullptr, &ps);
    context->PSSetShader(ps, nullptr, 0);

    context->Draw(4, 0);

    auto px = Readback(device, context, rtTex, W, H);
    auto result = CheckGolden("draw_tess_quad_simple", px.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    int redPixels = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (px[i*4+0] > 0.9f && px[i*4+1] < 0.1f && px[i*4+2] < 0.1f) { ++redPixels; }
    }
    EXPECT_GT(redPixels, 5000) << "Expected tessellated quad to fill most of the viewport";

    ps->Release(); ds->Release(); hs->Release(); vs->Release();
    layout->Release(); vb->Release(); rtv->Release(); rtTex->Release();
}

TEST_F(DrawGoldenTests, TessTriColor)
{
    const UINT W = 128, H = 128;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11Texture2D* rtTex = CreateRT(device, W, H, &rtv);
    FLOAT clear[4] = {0,0,0,1};
    context->ClearRenderTargetView(rtv, clear);
    context->OMSetRenderTargets(1, &rtv, nullptr);
    SetViewport(context, W, H);

    float verts[] = {
         0.0f,  0.7f, 0.5f,  1,0,0,1,
         0.7f, -0.7f, 0.5f,  0,1,0,1,
        -0.7f, -0.7f, 0.5f,  0,0,1,1,
    };

    D3D11_BUFFER_DESC bd{}; bd.ByteWidth = sizeof(verts); bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA sd{}; sd.pSysMem = verts;
    ID3D11Buffer* vb = nullptr; device->CreateBuffer(&bd, &sd, &vb);
    UINT stride = 28, offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

    D3D11_INPUT_ELEMENT_DESC il[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    auto vsBC = ReadBytecode(D3D11SW_SHADER_DIR "/vs_color.o");
    ASSERT_FALSE(vsBC.empty());
    ID3D11InputLayout* layout = nullptr; device->CreateInputLayout(il, 2, vsBC.data(), vsBC.size(), &layout);
    context->IASetInputLayout(layout);

    ID3D11VertexShader* vs = nullptr; device->CreateVertexShader(vsBC.data(), vsBC.size(), nullptr, &vs);
    context->VSSetShader(vs, nullptr, 0);

    auto hsBC = ReadBytecode(D3D11SW_SHADER_DIR "/hs_color_tri.o");
    ASSERT_FALSE(hsBC.empty()) << "HS not compiled — run compile.sh";
    ID3D11HullShader* hs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateHullShader(hsBC.data(), hsBC.size(), nullptr, &hs)));
    context->HSSetShader(hs, nullptr, 0);

    auto dsBC = ReadBytecode(D3D11SW_SHADER_DIR "/ds_color_tri.o");
    ASSERT_FALSE(dsBC.empty()) << "DS not compiled — run compile.sh";
    ID3D11DomainShader* ds = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDomainShader(dsBC.data(), dsBC.size(), nullptr, &ds)));
    context->DSSetShader(ds, nullptr, 0);

    auto psBC = ReadBytecode(D3D11SW_SHADER_DIR "/ps_color.o");
    ID3D11PixelShader* ps = nullptr; device->CreatePixelShader(psBC.data(), psBC.size(), nullptr, &ps);
    context->PSSetShader(ps, nullptr, 0);

    context->Draw(3, 0);

    auto px = Readback(device, context, rtTex, W, H);
    auto result = CheckGolden("draw_tess_tri_color", px.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    bool hasRed = false, hasGreen = false, hasBlue = false;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (px[i*4+0] > 0.5f && px[i*4+1] < 0.3f && px[i*4+2] < 0.3f) { hasRed = true; }
        if (px[i*4+1] > 0.5f && px[i*4+0] < 0.3f && px[i*4+2] < 0.3f) { hasGreen = true; }
        if (px[i*4+2] > 0.5f && px[i*4+0] < 0.3f && px[i*4+1] < 0.3f) { hasBlue = true; }
    }
    EXPECT_TRUE(hasRed && hasGreen && hasBlue)
        << "Expected interpolated R, G, B regions from tessellated triangle";

    ps->Release(); ds->Release(); hs->Release(); vs->Release();
    layout->Release(); vb->Release(); rtv->Release(); rtTex->Release();
}

TEST_F(DrawGoldenTests, TessTriColorGS)
{
    const UINT W = 128, H = 128;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11Texture2D* rtTex = CreateRT(device, W, H, &rtv);
    FLOAT clear[4] = {0,0,0,1};
    context->ClearRenderTargetView(rtv, clear);
    context->OMSetRenderTargets(1, &rtv, nullptr);
    SetViewport(context, W, H);

    float verts[] = {
         0.0f,  0.7f, 0.5f,  1,0,0,1,
         0.7f, -0.7f, 0.5f,  0,1,0,1,
        -0.7f, -0.7f, 0.5f,  0,0,1,1,
    };

    D3D11_BUFFER_DESC bd{}; bd.ByteWidth = sizeof(verts); bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA sd{}; sd.pSysMem = verts;
    ID3D11Buffer* vb = nullptr; device->CreateBuffer(&bd, &sd, &vb);
    UINT stride = 28, offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

    D3D11_INPUT_ELEMENT_DESC il[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    auto vsBC = ReadBytecode(D3D11SW_SHADER_DIR "/vs_color.o");
    ASSERT_FALSE(vsBC.empty());
    ID3D11InputLayout* layout = nullptr; device->CreateInputLayout(il, 2, vsBC.data(), vsBC.size(), &layout);
    context->IASetInputLayout(layout);

    ID3D11VertexShader* vs = nullptr; device->CreateVertexShader(vsBC.data(), vsBC.size(), nullptr, &vs);
    context->VSSetShader(vs, nullptr, 0);

    auto hsBC = ReadBytecode(D3D11SW_SHADER_DIR "/hs_color_tri.o");
    ASSERT_FALSE(hsBC.empty());
    ID3D11HullShader* hs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateHullShader(hsBC.data(), hsBC.size(), nullptr, &hs)));
    context->HSSetShader(hs, nullptr, 0);

    auto dsBC = ReadBytecode(D3D11SW_SHADER_DIR "/ds_color_tri.o");
    ASSERT_FALSE(dsBC.empty());
    ID3D11DomainShader* ds = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDomainShader(dsBC.data(), dsBC.size(), nullptr, &ds)));
    context->DSSetShader(ds, nullptr, 0);

    auto gsBC = ReadBytecode(D3D11SW_SHADER_DIR "/gs_explode.o");
    ASSERT_FALSE(gsBC.empty());
    ID3D11GeometryShader* gs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateGeometryShader(gsBC.data(), gsBC.size(), nullptr, &gs)));
    context->GSSetShader(gs, nullptr, 0);

    auto psBC = ReadBytecode(D3D11SW_SHADER_DIR "/ps_color.o");
    ID3D11PixelShader* ps = nullptr; device->CreatePixelShader(psBC.data(), psBC.size(), nullptr, &ps);
    context->PSSetShader(ps, nullptr, 0);

    context->Draw(3, 0);

    auto px = Readback(device, context, rtTex, W, H);
    auto result = CheckGolden("draw_tess_tri_color_gs", px.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    int coloredPixels = 0, blackPixels = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        float r = px[i*4+0], g = px[i*4+1], b = px[i*4+2];
        if (r > 0.1f || g > 0.1f || b > 0.1f) { ++coloredPixels; }
        else { ++blackPixels; }
    }
    EXPECT_GT(coloredPixels, 200) << "Expected tessellated+GS exploded triangles to produce colored pixels";
    EXPECT_GT(blackPixels, 1000) << "Expected gaps between exploded triangles (GS shrinks them)";

    ps->Release(); gs->Release(); ds->Release(); hs->Release(); vs->Release();
    layout->Release(); vb->Release(); rtv->Release(); rtTex->Release();
}
