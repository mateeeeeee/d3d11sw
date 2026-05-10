#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <bit>

#include "golden_test_util.h"

namespace
{
    constexpr UINT kW = 64;
    constexpr UINT kH = 64;

    IDirect3DDevice9* MakeHeadless()
    {
        IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
        D3DPRESENT_PARAMETERS pp = {};
        pp.BackBufferWidth  = kW; pp.BackBufferHeight = kH;
        pp.BackBufferFormat = D3DFMT_A8R8G8B8;
        pp.BackBufferCount  = 1; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.Windowed = TRUE;
        IDirect3DDevice9* dev = nullptr;
        d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                          D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
        d3d->Release();
        return dev;
    }

    std::vector<float> Readback(IDirect3DDevice9* dev)
    {
        IDirect3DSurface9* bb = nullptr;
        dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
        D3DLOCKED_RECT lr = {};
        bb->LockRect(&lr, nullptr, D3DLOCK_READONLY);
        std::vector<float> px(kW * kH * 4);
        for (UINT y = 0; y < kH; ++y)
        {
            const uint8_t* row = static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch;
            for (UINT x = 0; x < kW; ++x)
            {
                UINT i = (y * kW + x) * 4;
                px[i+0] = row[x*4+2] / 255.f;
                px[i+1] = row[x*4+1] / 255.f;
                px[i+2] = row[x*4+0] / 255.f;
                px[i+3] = row[x*4+3] / 255.f;
            }
        }
        bb->UnlockRect();
        bb->Release();
        return px;
    }
}

struct D3D9FogGoldenTests : ::testing::Test {};

// Linear vertex fog on a FF triangle.
// Vertices: top at Z=0 (no fog → red), bottom at Z=1 (full fog → white fog colour).
// Expected: gradient from red at top to white at bottom.
TEST_F(D3D9FogGoldenTests, VertexFog_Linear_RedToWhite)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0);
    dev->SetRenderState(D3DRS_CULLMODE,       D3DCULL_NONE);
    dev->SetRenderState(D3DRS_LIGHTING,       FALSE);
    dev->SetRenderState(D3DRS_FOGENABLE,      TRUE);
    dev->SetRenderState(D3DRS_FOGCOLOR,       0xFFFFFFFFu);   // white fog
    dev->SetRenderState(D3DRS_FOGVERTEXMODE,  D3DFOG_LINEAR);
    dev->SetRenderState(D3DRS_FOGTABLEMODE,   D3DFOG_NONE);
    dev->SetRenderState(D3DRS_FOGSTART,       std::bit_cast<DWORD>(0.0f));
    dev->SetRenderState(D3DRS_FOGEND,         std::bit_cast<DWORD>(1.0f));

    // Identity WVP: eye-space Z == vertex Z
    D3DMATRIX I = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    dev->SetTransform(D3DTS_WORLD,      &I);
    dev->SetTransform(D3DTS_VIEW,       &I);
    dev->SetTransform(D3DTS_PROJECTION, &I);

    // TSS: pass diffuse colour through (texture unbound → white fallback × diffuse = diffuse)
    dev->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
    dev->SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

    // Triangle: top vertex at Z=0 (no fog), bottom vertices at Z=1 (full fog)
    // All vertices are red.
    struct V { float x, y, z; D3DCOLOR color; };
    const V verts[3] = {
        {  0.0f,  0.9f, 0.0f, 0xFFFF0000u },   // top,   Z=0, fog_factor=1 → red
        { -0.9f, -0.9f, 1.0f, 0xFFFF0000u },   // btm-L, Z=1, fog_factor=0 → white
        {  0.9f, -0.9f, 1.0f, 0xFFFF0000u },   // btm-R, Z=1, fog_factor=0 → white
    };
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts),0,0,D3DPOOL_DEFAULT,&vb,nullptr), S_OK);
    void* m = nullptr;
    ASSERT_EQ(vb->Lock(0,sizeof(verts),&m,0), S_OK);
    std::memcpy(m, verts, sizeof(verts));
    ASSERT_EQ(vb->Unlock(), S_OK);
    dev->SetStreamSource(0, vb, 0, sizeof(V));
    dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    dev->SetVertexShader(nullptr);
    dev->SetPixelShader(nullptr);

    dev->BeginScene();
    dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
    dev->EndScene();

    auto pixels = Readback(dev);
    auto result = CheckGolden("d3d9_ff_fog_vertex_linear_64x64", pixels.data(), kW, kH, 1.f/255.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Sanity: top of triangle should be red (no fog), bottom should be white (full fog)
    auto px = [&](UINT x, UINT y) { return pixels.data() + (y * kW + x) * 4; };
    const float* top = px(kW/2, 4);
    EXPECT_GT(top[0], 0.7f) << "Top R should be high (near red)";
    EXPECT_LT(top[1], 0.3f) << "Top G should be low";
    EXPECT_LT(top[2], 0.3f) << "Top B should be low";

    const float* bot = px(kW/2, kH - 5);
    EXPECT_GT(bot[0], 0.7f) << "Bottom R should be high (white fog)";
    EXPECT_GT(bot[1], 0.7f) << "Bottom G should be high (white fog)";
    EXPECT_GT(bot[2], 0.7f) << "Bottom B should be high (white fog)";

    vb->Release();
    dev->Release();
}
