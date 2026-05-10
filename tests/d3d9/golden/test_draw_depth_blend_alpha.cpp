#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <cstdint>
#include <cstring>
#include <vector>

#include "golden_test_util.h"

namespace
{
    constexpr UINT kW = 64;
    constexpr UINT kH = 64;

    IDirect3DDevice9* MakeHeadless()
    {
        IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
        D3DPRESENT_PARAMETERS pp = {};
        pp.BackBufferWidth  = kW;  pp.BackBufferHeight = kH;
        pp.BackBufferFormat = D3DFMT_A8R8G8B8;
        pp.BackBufferCount  = 1;
        pp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
        pp.Windowed         = TRUE;
        IDirect3DDevice9* dev = nullptr;
        d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                          D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
        d3d->Release();
        return dev;
    }

    // Create and bind a D24S8 depth-stencil surface; returns it (caller Release's it).
    IDirect3DSurface9* AttachDepth(IDirect3DDevice9* dev)
    {
        IDirect3DSurface9* dsv = nullptr;
        dev->CreateDepthStencilSurface(kW, kH, D3DFMT_D24S8,
                                       D3DMULTISAMPLE_NONE, 0, TRUE, &dsv, nullptr);
        dev->SetDepthStencilSurface(dsv);
        return dsv;
    }

    // Read the backbuffer into a float RGBA array.
    void ReadBB(IDirect3DDevice9* dev, std::vector<float>& out)
    {
        out.resize(kW * kH * 4);
        IDirect3DSurface9* bb = nullptr;
        dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
        D3DLOCKED_RECT lr = {};
        bb->LockRect(&lr, nullptr, D3DLOCK_READONLY);
        for (UINT y = 0; y < kH; ++y)
        {
            const uint8_t* row = static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch;
            for (UINT x = 0; x < kW; ++x)
            {
                UINT i = (y * kW + x) * 4;
                out[i+0] = row[x*4+2] / 255.f;  // R
                out[i+1] = row[x*4+1] / 255.f;  // G
                out[i+2] = row[x*4+0] / 255.f;  // B
                out[i+3] = row[x*4+3] / 255.f;  // A
            }
        }
        bb->UnlockRect();
        bb->Release();
    }

    const float* Center(const std::vector<float>& px)
    {
        return px.data() + (kH/2 * kW + kW/2) * 4;
    }

    struct XyzDiffuse { float x, y, z; D3DCOLOR color; };

    IDirect3DVertexBuffer9* MakeVB(IDirect3DDevice9* dev,
                                    const XyzDiffuse* verts, UINT count)
    {
        IDirect3DVertexBuffer9* vb = nullptr;
        UINT bytes = sizeof(XyzDiffuse) * count;
        dev->CreateVertexBuffer(bytes, 0, 0, D3DPOOL_DEFAULT, &vb, nullptr);
        void* m = nullptr;
        vb->Lock(0, bytes, &m, 0);
        std::memcpy(m, verts, bytes);
        vb->Unlock();
        return vb;
    }
}

struct D3D9DrawDepthBlendAlphaTests : ::testing::Test
{
    IDirect3DDevice9* dev = nullptr;
    void SetUp()    override { dev = MakeHeadless(); ASSERT_NE(dev, nullptr); }
    void TearDown() override { if (dev) { dev->Release(); dev = nullptr; } }
};

// ---------------------------------------------------------------------------
// Depth test: two overlapping triangles at different depths.
// Near triangle (z=0.3, red) should occlude far triangle (z=0.7, blue).
// ---------------------------------------------------------------------------
TEST_F(D3D9DrawDepthBlendAlphaTests, DepthTest_NearOccludesFar)
{
    IDirect3DSurface9* dsv = AttachDepth(dev);

    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                         D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_ZENABLE,  D3DZB_TRUE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_ZFUNC,    D3DCMP_LESSEQUAL), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING, FALSE), S_OK);

    // Far blue triangle (z=0.7) — drawn first but should lose depth test at center
    const XyzDiffuse far[3] = {
        { -0.9f, -0.9f, 0.7f, 0xFF0000FFu },
        {  0.0f,  0.9f, 0.7f, 0xFF0000FFu },
        {  0.9f, -0.9f, 0.7f, 0xFF0000FFu },
    };
    // Near red triangle (z=0.3) — drawn second, wins depth test
    const XyzDiffuse near_[3] = {
        { -0.5f, -0.5f, 0.3f, 0xFFFF0000u },
        {  0.0f,  0.5f, 0.3f, 0xFFFF0000u },
        {  0.5f, -0.5f, 0.3f, 0xFFFF0000u },
    };

    // Create both VBs outside BeginScene (correct D3D9 usage)
    IDirect3DVertexBuffer9* farVB  = MakeVB(dev, far,   3);
    IDirect3DVertexBuffer9* nearVB = MakeVB(dev, near_, 3);

    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE), S_OK);
    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    dev->BeginScene();

    dev->SetStreamSource(0, farVB, 0, sizeof(XyzDiffuse));
    dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

    dev->SetStreamSource(0, nearVB, 0, sizeof(XyzDiffuse));
    dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

    dev->EndScene();

    std::vector<float> px;
    ReadBB(dev, px);
    auto result = CheckGolden("d3d9_depth_near_over_far_64x64", px.data(), kW, kH, 1.f/255.f);
    EXPECT_TRUE(result.passed) << result.message;

    const float* ctr = Center(px);
    EXPECT_GT(ctr[0], 0.9f) << "center R should be ~1 (near red wins depth test)";
    EXPECT_LT(ctr[2], 0.1f) << "center B should be ~0";

    farVB->Release();
    nearVB->Release();
    dsv->Release();
}

// ---------------------------------------------------------------------------
// Depth write disabled: near triangle drawn first without writing depth, so a
// subsequent far triangle still passes the depth test and overwrites it.
// ---------------------------------------------------------------------------
TEST_F(D3D9DrawDepthBlendAlphaTests, DepthTest_WriteDisabled_FarOverwritesNear)
{
    IDirect3DSurface9* dsv = AttachDepth(dev);

    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                         D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE,     D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_ZENABLE,      D3DZB_TRUE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_ZFUNC,        D3DCMP_LESSEQUAL), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING,     FALSE), S_OK);

    const XyzDiffuse red[3]  = {
        { -0.5f, -0.5f, 0.3f, 0xFFFF0000u },
        {  0.0f,  0.5f, 0.3f, 0xFFFF0000u },
        {  0.5f, -0.5f, 0.3f, 0xFFFF0000u },
    };
    const XyzDiffuse blue[3] = {
        { -0.5f, -0.5f, 0.7f, 0xFF0000FFu },
        {  0.0f,  0.5f, 0.7f, 0xFF0000FFu },
        {  0.5f, -0.5f, 0.7f, 0xFF0000FFu },
    };

    IDirect3DVertexBuffer9* redVB  = MakeVB(dev, red,  3);
    IDirect3DVertexBuffer9* blueVB = MakeVB(dev, blue, 3);

    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE), S_OK);
    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    dev->BeginScene();

    // Draw near red with depth writes OFF — colour visible but depth stays 1.0
    ASSERT_EQ(dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE), S_OK);
    dev->SetStreamSource(0, redVB, 0, sizeof(XyzDiffuse));
    dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

    // Draw far blue with depth writes ON — depth still 1.0, so 0.7 ≤ 1.0 passes
    ASSERT_EQ(dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE), S_OK);
    dev->SetStreamSource(0, blueVB, 0, sizeof(XyzDiffuse));
    dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

    dev->EndScene();

    std::vector<float> px;
    ReadBB(dev, px);
    auto result = CheckGolden("d3d9_depth_write_disabled_64x64", px.data(), kW, kH, 1.f/255.f);
    EXPECT_TRUE(result.passed) << result.message;

    const float* ctr = Center(px);
    EXPECT_GT(ctr[2], 0.9f) << "center B should be ~1 (far blue overwrote near red)";
    EXPECT_LT(ctr[0], 0.1f) << "center R should be ~0";

    redVB->Release();
    blueVB->Release();
    dsv->Release();
}

// ---------------------------------------------------------------------------
// Alpha blending — additive (SRCBLEND=ONE, DESTBLEND=ONE).
// Clear to dark blue (0,0,128). Draw dark red (128,0,0). Result: purple (~128,0,128).
// ---------------------------------------------------------------------------
TEST_F(D3D9DrawDepthBlendAlphaTests, AlphaBlend_Additive)
{
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET,
                         D3DCOLOR_ARGB(0xFF, 0, 0, 128), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE,        D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING,        FALSE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_SRCBLEND,        D3DBLEND_ONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_DESTBLEND,       D3DBLEND_ONE), S_OK);

    const XyzDiffuse verts[3] = {
        { -0.8f, -0.8f, 0.5f, D3DCOLOR_ARGB(0xFF, 128, 0, 0) },
        {  0.0f,  0.8f, 0.5f, D3DCOLOR_ARGB(0xFF, 128, 0, 0) },
        {  0.8f, -0.8f, 0.5f, D3DCOLOR_ARGB(0xFF, 128, 0, 0) },
    };
    IDirect3DVertexBuffer9* vb = MakeVB(dev, verts, 3);

    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(XyzDiffuse)), S_OK);
    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE), S_OK);
    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    dev->BeginScene();
    dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
    dev->EndScene();

    std::vector<float> px;
    ReadBB(dev, px);
    auto result = CheckGolden("d3d9_blend_additive_64x64", px.data(), kW, kH, 2.f/255.f);
    EXPECT_TRUE(result.passed) << result.message;

    const float* ctr = Center(px);
    EXPECT_GT(ctr[0], 0.3f) << "center R should be > 0 (dark red added)";
    EXPECT_GT(ctr[2], 0.3f) << "center B should be > 0 (dark blue background)";
    EXPECT_LT(ctr[1], 0.1f) << "center G should be ~0";

    vb->Release();
}

// ---------------------------------------------------------------------------
// Alpha blending — SRC_ALPHA / INV_SRC_ALPHA.
// Clear to blue (0,0,255). Draw red with alpha≈50% → ~50% red + ~50% blue.
// ---------------------------------------------------------------------------
TEST_F(D3D9DrawDepthBlendAlphaTests, AlphaBlend_SrcAlpha)
{
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET,
                         D3DCOLOR_ARGB(0xFF, 0, 0, 255), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE,        D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING,        FALSE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_SRCBLEND,        D3DBLEND_SRCALPHA), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_DESTBLEND,       D3DBLEND_INVSRCALPHA), S_OK);

    // Route vertex diffuse alpha to PS output alpha
    ASSERT_EQ(dev->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1), S_OK);
    ASSERT_EQ(dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE), S_OK);

    const XyzDiffuse verts[3] = {
        { -0.8f, -0.8f, 0.5f, D3DCOLOR_ARGB(127, 255, 0, 0) },
        {  0.0f,  0.8f, 0.5f, D3DCOLOR_ARGB(127, 255, 0, 0) },
        {  0.8f, -0.8f, 0.5f, D3DCOLOR_ARGB(127, 255, 0, 0) },
    };
    IDirect3DVertexBuffer9* vb = MakeVB(dev, verts, 3);

    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(XyzDiffuse)), S_OK);
    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE), S_OK);
    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    dev->BeginScene();
    dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
    dev->EndScene();

    std::vector<float> px;
    ReadBB(dev, px);
    auto result = CheckGolden("d3d9_blend_srcalpha_64x64", px.data(), kW, kH, 2.f/255.f);
    EXPECT_TRUE(result.passed) << result.message;

    const float* ctr = Center(px);
    EXPECT_GT(ctr[0], 0.3f) << "center R should be > 0 (50% red source)";
    EXPECT_GT(ctr[2], 0.3f) << "center B should be > 0 (50% blue dest)";
    EXPECT_LT(ctr[1], 0.1f) << "center G should be ~0";

    vb->Release();
}

// ---------------------------------------------------------------------------
// Alpha test — GREATEREQUAL with ref=127.
// Left triangle (alpha=255) passes → drawn red.
// Right triangle (alpha=0) fails → discarded; background blue shows.
// ---------------------------------------------------------------------------
TEST_F(D3D9DrawDepthBlendAlphaTests, AlphaTest_GreaterEqualRef)
{
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET,
                         D3DCOLOR_ARGB(0xFF, 0, 0, 255), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE,         D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING,         FALSE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_ALPHATESTENABLE,  TRUE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_ALPHAREF,         127), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_ALPHAFUNC,        D3DCMP_GREATEREQUAL), S_OK);

    // Route vertex diffuse alpha to PS output alpha
    ASSERT_EQ(dev->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1), S_OK);
    ASSERT_EQ(dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE), S_OK);

    // Left half: red with alpha=255 (PASSES alpha test → drawn)
    // Triangle covers screen x=[0,32], verified to contain (16,32).
    const XyzDiffuse passVerts[3] = {
        { -1.0f, -1.0f, 0.5f, D3DCOLOR_ARGB(255, 255, 0, 0) },
        { -0.5f,  1.0f, 0.5f, D3DCOLOR_ARGB(255, 255, 0, 0) },
        {  0.0f, -1.0f, 0.5f, D3DCOLOR_ARGB(255, 255, 0, 0) },
    };
    // Right half: green with alpha=0 (FAILS alpha test → discarded)
    // Triangle covers screen x=[32,64], verified to contain (48,32).
    const XyzDiffuse failVerts[3] = {
        {  0.0f, -1.0f, 0.5f, D3DCOLOR_ARGB(0, 0, 255, 0) },
        {  0.5f,  1.0f, 0.5f, D3DCOLOR_ARGB(0, 0, 255, 0) },
        {  1.0f, -1.0f, 0.5f, D3DCOLOR_ARGB(0, 0, 255, 0) },
    };

    IDirect3DVertexBuffer9* passVB = MakeVB(dev, passVerts, 3);
    IDirect3DVertexBuffer9* failVB = MakeVB(dev, failVerts, 3);

    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE), S_OK);
    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    dev->BeginScene();

    dev->SetStreamSource(0, passVB, 0, sizeof(XyzDiffuse));
    dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

    dev->SetStreamSource(0, failVB, 0, sizeof(XyzDiffuse));
    dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

    dev->EndScene();

    std::vector<float> px;
    ReadBB(dev, px);
    auto result = CheckGolden("d3d9_alpha_test_64x64", px.data(), kW, kH, 1.f/255.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Left quarter: red (alpha=255 passes)
    const float* left = px.data() + (kH/2 * kW + kW/4) * 4;
    EXPECT_GT(left[0], 0.9f) << "left R should be ~1 (alpha=255 passes)";
    EXPECT_LT(left[2], 0.1f) << "left B should be ~0";

    // Right three-quarter: blue background (alpha=0 discarded)
    const float* right = px.data() + (kH/2 * kW + 3*kW/4) * 4;
    EXPECT_GT(right[2], 0.9f) << "right B should be ~1 (bg, green discarded)";
    EXPECT_LT(right[1], 0.1f) << "right G should be ~0 (green discarded)";

    passVB->Release();
    failVB->Release();
}
