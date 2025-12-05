#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "perf_util.h"
#include "../golden/golden_test_util.h"

struct Vertex
{
    float x, y, z;
    float r, g, b, a;
};

static const Vertex kTriangle[] = {
    { 0.0f,  0.8f, 0.5f,  1.f, 0.f, 0.f, 1.f},
    {-0.8f, -0.8f, 0.5f,  0.f, 1.f, 0.f, 1.f},
    { 0.8f, -0.8f, 0.5f,  0.f, 0.f, 1.f, 1.f},
};

static const Vertex kTriangleAlpha[] = {
    { 0.0f,  0.8f, 0.5f,  1.f, 0.f, 0.f, 0.5f},
    {-0.8f, -0.8f, 0.5f,  0.f, 1.f, 0.f, 0.5f},
    { 0.8f, -0.8f, 0.5f,  0.f, 0.f, 1.f, 0.5f},
};

struct PerfDraw : ::testing::Test
{
    ID3D11Device*        device      = nullptr;
    ID3D11DeviceContext* context     = nullptr;
    ID3D11VertexShader*  vs         = nullptr;
    ID3D11PixelShader*   ps         = nullptr;
    ID3D11InputLayout*   layout     = nullptr;
    ID3D11Buffer*        vb         = nullptr;
    ID3D11Buffer*        vbAlpha    = nullptr;
    ID3D11Buffer*        vbMany     = nullptr;

    static constexpr int kManyTriangles = 100;

    void SetUp() override
    {
        D3D_FEATURE_LEVEL fl;
        ASSERT_TRUE(SUCCEEDED(D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION, &device, &fl, &context)));

        auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_color.o");
        ASSERT_FALSE(vsBytecode.empty()) << "vs_color.o not found — run compile.sh";
        auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_color.o");
        ASSERT_FALSE(psBytecode.empty()) << "ps_color.o not found — run compile.sh";

        ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
            vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));
        ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
            psBytecode.data(), psBytecode.size(), nullptr, &ps)));

        D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
            layoutDesc, 2, vsBytecode.data(), vsBytecode.size(), &layout)));

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = sizeof(kTriangle);
        vbDesc.Usage     = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vbInit{};
        vbInit.pSysMem = kTriangle;
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

        vbDesc.ByteWidth = sizeof(kTriangleAlpha);
        D3D11_SUBRESOURCE_DATA vbAlphaInit{};
        vbAlphaInit.pSysMem = kTriangleAlpha;
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbAlphaInit, &vbAlpha)));

        std::vector<Vertex> manyVerts(kManyTriangles * 3);
        for (int i = 0; i < kManyTriangles; ++i)
        {
            float cx = ((i % 10) - 4.5f) / 5.0f;
            float cy = ((i / 10) - 4.5f) / 5.0f;
            float s  = 0.08f;
            manyVerts[i * 3 + 0] = {cx,       cy + s, 0.5f,  1.f, 0.f, 0.f, 1.f};
            manyVerts[i * 3 + 1] = {cx - s, cy - s, 0.5f,  0.f, 1.f, 0.f, 1.f};
            manyVerts[i * 3 + 2] = {cx + s, cy - s, 0.5f,  0.f, 0.f, 1.f, 1.f};
        }

        D3D11_BUFFER_DESC manyDesc{};
        manyDesc.ByteWidth = static_cast<UINT>(manyVerts.size() * sizeof(Vertex));
        manyDesc.Usage     = D3D11_USAGE_DEFAULT;
        manyDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA manyInit{};
        manyInit.pSysMem = manyVerts.data();
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&manyDesc, &manyInit, &vbMany)));

        context->IASetInputLayout(layout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(vs, nullptr, 0);
        context->PSSetShader(ps, nullptr, 0);
    }

    void TearDown() override
    {
        if (vbMany)  { vbMany->Release(); }
        if (vbAlpha) { vbAlpha->Release(); }
        if (vb)      { vb->Release(); }
        if (layout)  { layout->Release(); }
        if (ps)      { ps->Release(); }
        if (vs)      { vs->Release(); }
        if (context) { context->Release(); }
        if (device)  { device->Release(); }
    }

    void BindVB(ID3D11Buffer* buf)
    {
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &buf, &stride, &offset);
    }

    void BindSingleTriangleVB()    { BindVB(vb); }
    void BindAlphaTriangleVB()     { BindVB(vbAlpha); }
    void BindManyTrianglesVB()     { BindVB(vbMany); }

    struct RTResources
    {
        ID3D11Texture2D*        tex      = nullptr;
        ID3D11RenderTargetView* rtv      = nullptr;
        ID3D11Texture2D*        depthTex = nullptr;
        ID3D11DepthStencilView* dsv      = nullptr;

        void Release()
        {
            if (dsv)      { dsv->Release(); dsv = nullptr; }
            if (depthTex) { depthTex->Release(); depthTex = nullptr; }
            if (rtv)      { rtv->Release(); rtv = nullptr; }
            if (tex)      { tex->Release(); tex = nullptr; }
        }
    };

    RTResources CreateRT(UINT w, UINT h)
    {
        RTResources rt;

        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width            = w;
        texDesc.Height           = h;
        texDesc.MipLevels        = 1;
        texDesc.ArraySize        = 1;
        texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage            = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags        = D3D11_BIND_RENDER_TARGET;
        device->CreateTexture2D(&texDesc, nullptr, &rt.tex);
        device->CreateRenderTargetView(rt.tex, nullptr, &rt.rtv);

        context->OMSetRenderTargets(1, &rt.rtv, nullptr);

        D3D11_VIEWPORT viewport{};
        viewport.Width    = static_cast<FLOAT>(w);
        viewport.Height   = static_cast<FLOAT>(h);
        viewport.MaxDepth = 1.f;
        context->RSSetViewports(1, &viewport);

        FLOAT clearColor[4] = {0.f, 0.f, 0.f, 1.f};
        context->ClearRenderTargetView(rt.rtv, clearColor);

        return rt;
    }

    RTResources CreateRTWithDepth(UINT w, UINT h)
    {
        RTResources rt;

        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width            = w;
        texDesc.Height           = h;
        texDesc.MipLevels        = 1;
        texDesc.ArraySize        = 1;
        texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage            = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags        = D3D11_BIND_RENDER_TARGET;
        device->CreateTexture2D(&texDesc, nullptr, &rt.tex);
        device->CreateRenderTargetView(rt.tex, nullptr, &rt.rtv);

        D3D11_TEXTURE2D_DESC dsDesc{};
        dsDesc.Width            = w;
        dsDesc.Height           = h;
        dsDesc.MipLevels        = 1;
        dsDesc.ArraySize        = 1;
        dsDesc.Format           = DXGI_FORMAT_D32_FLOAT;
        dsDesc.SampleDesc.Count = 1;
        dsDesc.Usage            = D3D11_USAGE_DEFAULT;
        dsDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;
        device->CreateTexture2D(&dsDesc, nullptr, &rt.depthTex);
        device->CreateDepthStencilView(rt.depthTex, nullptr, &rt.dsv);

        context->OMSetRenderTargets(1, &rt.rtv, rt.dsv);

        D3D11_VIEWPORT viewport{};
        viewport.Width    = static_cast<FLOAT>(w);
        viewport.Height   = static_cast<FLOAT>(h);
        viewport.MaxDepth = 1.f;
        context->RSSetViewports(1, &viewport);

        FLOAT clearColor[4] = {0.f, 0.f, 0.f, 1.f};
        context->ClearRenderTargetView(rt.rtv, clearColor);
        context->ClearDepthStencilView(rt.dsv, D3D11_CLEAR_DEPTH, 1.f, 0);

        return rt;
    }

    ID3D11BlendState* CreateAlphaBlendState()
    {
        D3D11_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable           = TRUE;
        blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        ID3D11BlendState* bs = nullptr;
        device->CreateBlendState(&blendDesc, &bs);
        return bs;
    }
};

// --- Basic draw benchmarks ---

TEST_F(PerfDraw, SingleTriangle_64x64)
{
    auto rt = CreateRT(64, 64);
    BindSingleTriangleVB();

    auto result = RunBenchmark("SingleTriangle_64x64", context, device, 100, 10, [&]() {
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
    rt.Release();
}

TEST_F(PerfDraw, SingleTriangle_256x256)
{
    auto rt = CreateRT(256, 256);
    BindSingleTriangleVB();

    auto result = RunBenchmark("SingleTriangle_256x256", context, device, 100, 10, [&]() {
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
    rt.Release();
}

TEST_F(PerfDraw, SingleTriangle_512x512)
{
    auto rt = CreateRT(512, 512);
    BindSingleTriangleVB();

    auto result = RunBenchmark("SingleTriangle_512x512", context, device, 50, 5, [&]() {
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
    rt.Release();
}

TEST_F(PerfDraw, ManyTriangles_64x64)
{
    auto rt = CreateRT(64, 64);
    BindManyTrianglesVB();

    auto result = RunBenchmark("ManyTriangles_64x64", context, device, 100, 10, [&]() {
        context->Draw(kManyTriangles * 3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
    rt.Release();
}

TEST_F(PerfDraw, SingleTriangle_1024x1024)
{
    auto rt = CreateRT(1024, 1024);
    BindSingleTriangleVB();

    auto result = RunBenchmark("SingleTriangle_1024x1024", context, device, 20, 3, [&]() {
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
    rt.Release();
}

TEST_F(PerfDraw, ThinTriangle_512x512)
{
    static const Vertex thinTri[] = {
        {-0.95f,  0.95f, 0.5f,  1.f, 0.f, 0.f, 1.f},
        { 0.95f, -0.95f, 0.5f,  0.f, 1.f, 0.f, 1.f},
        {-0.90f,  0.90f, 0.5f,  0.f, 0.f, 1.f, 1.f},
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(thinTri);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = thinTri;
    ID3D11Buffer* thinVB = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &thinVB)));

    BindVB(thinVB);
    auto rt = CreateRT(512, 512);

    auto result = RunBenchmark("ThinTriangle_512x512", context, device, 50, 5, [&]() {
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
    rt.Release();
    thinVB->Release();
}

// --- Depth benchmarks ---

TEST_F(PerfDraw, DepthWrite_256x256)
{
    BindSingleTriangleVB();

    D3D11_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthEnable    = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc      = D3D11_COMPARISON_LESS;
    ID3D11DepthStencilState* dsState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilState(&dsDesc, &dsState)));
    context->OMSetDepthStencilState(dsState, 0);

    auto rt = CreateRTWithDepth(256, 256);

    auto result = RunBenchmark("DepthWrite_256x256", context, device, 100, 10, [&]() {
        context->ClearDepthStencilView(rt.dsv, D3D11_CLEAR_DEPTH, 1.f, 0);
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);

    dsState->Release();
    rt.Release();
}

TEST_F(PerfDraw, DepthReject_256x256)
{
    static const Vertex nearTri[] = {
        { 0.0f,  0.8f, 0.2f,  1.f, 0.f, 0.f, 1.f},
        {-0.8f, -0.8f, 0.2f,  0.f, 1.f, 0.f, 1.f},
        { 0.8f, -0.8f, 0.2f,  0.f, 0.f, 1.f, 1.f},
    };
    static const Vertex farTri[] = {
        { 0.0f,  0.8f, 0.8f,  1.f, 1.f, 0.f, 1.f},
        {-0.8f, -0.8f, 0.8f,  1.f, 1.f, 0.f, 1.f},
        { 0.8f, -0.8f, 0.8f,  1.f, 1.f, 0.f, 1.f},
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(nearTri);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA nearInit{};
    nearInit.pSysMem = nearTri;
    ID3D11Buffer* nearVB = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &nearInit, &nearVB)));

    vbDesc.ByteWidth = sizeof(farTri);
    D3D11_SUBRESOURCE_DATA farInit{};
    farInit.pSysMem = farTri;
    ID3D11Buffer* farVB = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &farInit, &farVB)));

    D3D11_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthEnable    = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc      = D3D11_COMPARISON_LESS;
    ID3D11DepthStencilState* dsState = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilState(&dsDesc, &dsState)));
    context->OMSetDepthStencilState(dsState, 0);

    auto rt = CreateRTWithDepth(256, 256);

    BindVB(nearVB);
    context->Draw(3, 0);

    BindVB(farVB);

    auto result = RunBenchmark("DepthReject_256x256", context, device, 100, 10, [&]() {
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);

    dsState->Release();
    farVB->Release();
    nearVB->Release();
    rt.Release();
}

// --- Blend benchmarks ---

TEST_F(PerfDraw, NoBlend_256x256)
{
    BindAlphaTriangleVB();
    auto rt = CreateRT(256, 256);

    auto result = RunBenchmark("NoBlend_256x256", context, device, 100, 10, [&]() {
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
    rt.Release();
}

TEST_F(PerfDraw, AlphaBlend_256x256)
{
    BindAlphaTriangleVB();
    ID3D11BlendState* bs = CreateAlphaBlendState();
    ASSERT_NE(bs, nullptr);
    FLOAT factor[4] = {0.f, 0.f, 0.f, 0.f};
    context->OMSetBlendState(bs, factor, 0xFFFFFFFF);

    auto rt = CreateRT(256, 256);

    auto result = RunBenchmark("AlphaBlend_256x256", context, device, 100, 10, [&]() {
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
    rt.Release();
    bs->Release();
}

TEST_F(PerfDraw, AdditiveBlend_256x256)
{
    BindAlphaTriangleVB();

    D3D11_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].BlendEnable           = TRUE;
    blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ID3D11BlendState* bs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBlendState(&blendDesc, &bs)));
    FLOAT factor[4] = {0.f, 0.f, 0.f, 0.f};
    context->OMSetBlendState(bs, factor, 0xFFFFFFFF);

    auto rt = CreateRT(256, 256);

    auto result = RunBenchmark("AdditiveBlend_256x256", context, device, 100, 10, [&]() {
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
    rt.Release();
    bs->Release();
}

TEST_F(PerfDraw, AlphaBlend_512x512)
{
    BindAlphaTriangleVB();
    ID3D11BlendState* bs = CreateAlphaBlendState();
    ASSERT_NE(bs, nullptr);
    FLOAT factor[4] = {0.f, 0.f, 0.f, 0.f};
    context->OMSetBlendState(bs, factor, 0xFFFFFFFF);

    auto rt = CreateRT(512, 512);

    auto result = RunBenchmark("AlphaBlend_512x512", context, device, 50, 5, [&]() {
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
    rt.Release();
    bs->Release();
}

TEST_F(PerfDraw, Overdraw4x_AlphaBlend_256x256)
{
    BindAlphaTriangleVB();
    ID3D11BlendState* bs = CreateAlphaBlendState();
    ASSERT_NE(bs, nullptr);
    FLOAT factor[4] = {0.f, 0.f, 0.f, 0.f};
    context->OMSetBlendState(bs, factor, 0xFFFFFFFF);

    auto rt = CreateRT(256, 256);

    auto result = RunBenchmark("Overdraw4x_AlphaBlend_256", context, device, 50, 5, [&]() {
        context->Draw(3, 0);
        context->Draw(3, 0);
        context->Draw(3, 0);
        context->Draw(3, 0);
    });
    PrintHeader();
    PrintResult(result);
    PrintFooter();
    WriteResult(result);
    EXPECT_GT(result.median_ns, 0.0);
    rt.Release();
    bs->Release();
}
