#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <cstdint>
#include <cstring>
#include <vector>

#include "golden_test_util.h"

// Tests for the fixed-function pipeline: no vertex or pixel shader set,
// using SetFVF + device transform matrices.

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
}

struct D3D9DrawGoldenTests : ::testing::Test
{
    IDirect3DDevice9* dev = nullptr;
    void SetUp() override   { dev = MakeHeadless(); ASSERT_NE(dev, nullptr); }
    void TearDown() override { if (dev) { dev->Release(); dev = nullptr; } }
};

// Fixed-function triangle with diffuse colour.
// WVP = identity (all default transform matrices), vertices in clip space.
// Triangle is CW-wound (D3D9 default cull=CCW keeps CW triangles).
TEST_F(D3D9DrawGoldenTests, FFTriangle_DiffuseColor)
{
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET,
                         D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE), S_OK);

    struct V { float x, y, z; D3DCOLOR color; };
    // CW in clip space, solid red (D3DCOLOR = ARGB = 0xFFFF0000)
    const V verts[3] = {
        { -0.8f, -0.8f, 0.5f, 0xFFFF0000u },
        {  0.0f,  0.8f, 0.5f, 0xFFFF0000u },
        {  0.8f, -0.8f, 0.5f, 0xFFFF0000u },
    };

    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts),0,0,D3DPOOL_DEFAULT,&vb,nullptr), S_OK);
    void* m = nullptr;
    ASSERT_EQ(vb->Lock(0,sizeof(verts),&m,0), S_OK);
    std::memcpy(m, verts, sizeof(verts));
    ASSERT_EQ(vb->Unlock(), S_OK);
    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(V)), S_OK);
    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE), S_OK);

    // No VS/PS — fixed-function path
    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    ASSERT_EQ(dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bb), S_OK);
    D3DLOCKED_RECT blr = {};
    ASSERT_EQ(bb->LockRect(&blr,nullptr,D3DLOCK_READONLY), S_OK);

    std::vector<float> pixels(kW * kH * 4);
    for (UINT y = 0; y < kH; ++y)
    {
        const uint8_t* row = static_cast<const uint8_t*>(blr.pBits) + y * blr.Pitch;
        for (UINT x = 0; x < kW; ++x)
        {
            UINT i = (y * kW + x) * 4;
            pixels[i+0] = row[x*4+2] / 255.f;
            pixels[i+1] = row[x*4+1] / 255.f;
            pixels[i+2] = row[x*4+0] / 255.f;
            pixels[i+3] = row[x*4+3] / 255.f;
        }
    }
    ASSERT_EQ(bb->UnlockRect(), S_OK);

    auto result = CheckGolden("d3d9_ff_triangle_64x64", pixels.data(), kW, kH, 0.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Center pixel should be red (from diffuse D3DCOLOR 0xFFFF0000 = ARGB red)
    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_GT(ctr[0], 0.9f) << "center R should be ~1.0 (red diffuse)";
    EXPECT_LT(ctr[1], 0.1f) << "center G should be ~0";
    EXPECT_LT(ctr[2], 0.1f) << "center B should be ~0";

    bb->Release();
    vb->Release();
}

// Directional-light lit triangle.
// FVF: XYZ + NORMAL (no vertex diffuse). One white directional light from above-front.
// Material: red diffuse. WVP = identity (clip-space vertices).
// Normals point toward the light (+Z direction in view space).
TEST_F(D3D9DrawGoldenTests, FFTriangle_DirectionalLight)
{
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET,
                         D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING, TRUE), S_OK);

    // Red diffuse material, black ambient/specular/emissive.
    D3DMATERIAL9 mat = {};
    mat.Diffuse  = { 1.f, 0.f, 0.f, 1.f };
    mat.Ambient  = { 0.f, 0.f, 0.f, 1.f };
    mat.Specular = { 0.f, 0.f, 0.f, 1.f };
    mat.Emissive = { 0.f, 0.f, 0.f, 1.f };
    mat.Power    = 1.f;
    ASSERT_EQ(dev->SetMaterial(&mat), S_OK);

    // Single white directional light pointing into -Z (from viewer toward screen).
    // In view space (identity view), -Z is toward the screen.
    // Normal = +Z, light dir = +Z → dot(N, -lightDir) = dot(+Z, -Z) = -1 → NdotL = 0?
    // Wait: for a directional light, L = normalize(-dir). If dir=(0,0,1), then L=(0,0,-1).
    // NdotL = dot(N=(0,0,1), L=(0,0,-1)) = -1 → clamped to 0 → black.
    // Instead use light dir=(0,0,-1) so L=(0,0,1), NdotL=1.
    D3DLIGHT9 light = {};
    light.Type      = D3DLIGHT_DIRECTIONAL;
    light.Diffuse   = { 1.f, 1.f, 1.f, 1.f };
    light.Direction = { 0.f, 0.f, -1.f };  // points into -Z; L = +Z
    ASSERT_EQ(dev->SetLight(0, &light), S_OK);
    ASSERT_EQ(dev->LightEnable(0, TRUE), S_OK);

    // Identity transforms — vertices already in clip space.
    // Normals = +Z = (0,0,1).
    struct V { float x, y, z;  float nx, ny, nz; };
    const V verts[3] = {
        { -0.8f, -0.8f, 0.5f,  0.f, 0.f, 1.f },
        {  0.0f,  0.8f, 0.5f,  0.f, 0.f, 1.f },
        {  0.8f, -0.8f, 0.5f,  0.f, 0.f, 1.f },
    };

    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);
    void* m = nullptr;
    ASSERT_EQ(vb->Lock(0, sizeof(verts), &m, 0), S_OK);
    std::memcpy(m, verts, sizeof(verts));
    ASSERT_EQ(vb->Unlock(), S_OK);
    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(V)), S_OK);
    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL), S_OK);

    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    ASSERT_EQ(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb), S_OK);
    D3DLOCKED_RECT blr = {};
    ASSERT_EQ(bb->LockRect(&blr, nullptr, D3DLOCK_READONLY), S_OK);

    std::vector<float> pixels(kW * kH * 4);
    for (UINT y = 0; y < kH; ++y)
    {
        const uint8_t* row = static_cast<const uint8_t*>(blr.pBits) + y * blr.Pitch;
        for (UINT x = 0; x < kW; ++x)
        {
            UINT i = (y * kW + x) * 4;
            pixels[i+0] = row[x*4+2] / 255.f;   // R
            pixels[i+1] = row[x*4+1] / 255.f;   // G
            pixels[i+2] = row[x*4+0] / 255.f;   // B
            pixels[i+3] = row[x*4+3] / 255.f;   // A
        }
    }
    ASSERT_EQ(bb->UnlockRect(), S_OK);

    auto result = CheckGolden("d3d9_ff_lit_triangle_64x64", pixels.data(), kW, kH, 1.f / 255.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Center pixel should be fully red (NdotL=1 × white light × red material)
    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_GT(ctr[0], 0.9f) << "center R should be ~1.0 (fully lit red)";
    EXPECT_LT(ctr[1], 0.1f) << "center G should be ~0";
    EXPECT_LT(ctr[2], 0.1f) << "center B should be ~0";

    bb->Release();
    vb->Release();
}

// Two-stage TSS: stage 0 = SELECTARG1 (texture), stage 1 = MODULATE (stage0 * diffuse).
TEST_F(D3D9DrawGoldenTests, FFTriangle_TwoStageModulate)
{
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET,
                         D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING, FALSE), S_OK);

    // 4×4 blue texture
    constexpr UINT texW = 4, texH = 4;
    uint32_t texels[texH * texW];
    for (UINT i = 0; i < texH * texW; ++i) { texels[i] = 0xFF0000FFu; }  // A8R8G8B8 blue

    IDirect3DTexture9* tex = nullptr;
    ASSERT_EQ(dev->CreateTexture(texW, texH, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, nullptr), S_OK);
    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(tex->LockRect(0, &lr, nullptr, 0), S_OK);
    for (UINT y = 0; y < texH; ++y)
    {
        std::memcpy(static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch,
                    texels + y * texW, texW * 4);
    }
    ASSERT_EQ(tex->UnlockRect(0), S_OK);

    // Stage 0: SELECTARG1 = texture
    ASSERT_EQ(dev->SetTexture(0, tex), S_OK);
    ASSERT_EQ(dev->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1), S_OK);
    ASSERT_EQ(dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE), S_OK);
    ASSERT_EQ(dev->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1), S_OK);
    ASSERT_EQ(dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE), S_OK);
    // Stage 1: MODULATE current * diffuse
    ASSERT_EQ(dev->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE), S_OK);
    ASSERT_EQ(dev->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT), S_OK);
    ASSERT_EQ(dev->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_DIFFUSE), S_OK);
    ASSERT_EQ(dev->SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1), S_OK);
    ASSERT_EQ(dev->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT), S_OK);
    ASSERT_EQ(dev->SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_DISABLE), S_OK);

    // White diffuse vertex color
    struct V { float x, y, z; D3DCOLOR color; float u, v; };
    const V verts[3] = {
        { -0.8f, -0.8f, 0.5f, 0xFFFFFFFFu, 0.f, 1.f },
        {  0.0f,  0.8f, 0.5f, 0xFFFFFFFFu, 0.5f, 0.f },
        {  0.8f, -0.8f, 0.5f, 0xFFFFFFFFu, 1.f, 1.f },
    };
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);
    void* mp = nullptr;
    ASSERT_EQ(vb->Lock(0, sizeof(verts), &mp, 0), S_OK);
    std::memcpy(mp, verts, sizeof(verts));
    ASSERT_EQ(vb->Unlock(), S_OK);
    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(V)), S_OK);
    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1), S_OK);
    ASSERT_EQ(dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT), S_OK);
    ASSERT_EQ(dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT), S_OK);

    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    ASSERT_EQ(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb), S_OK);
    D3DLOCKED_RECT blr = {};
    ASSERT_EQ(bb->LockRect(&blr, nullptr, D3DLOCK_READONLY), S_OK);

    std::vector<float> pixels(kW * kH * 4);
    for (UINT y = 0; y < kH; ++y)
    {
        const uint8_t* row = static_cast<const uint8_t*>(blr.pBits) + y * blr.Pitch;
        for (UINT x = 0; x < kW; ++x)
        {
            UINT i = (y * kW + x) * 4;
            pixels[i+0] = row[x*4+2] / 255.f;
            pixels[i+1] = row[x*4+1] / 255.f;
            pixels[i+2] = row[x*4+0] / 255.f;
            pixels[i+3] = row[x*4+3] / 255.f;
        }
    }
    ASSERT_EQ(bb->UnlockRect(), S_OK);

    auto result = CheckGolden("d3d9_ff_two_stage_64x64", pixels.data(), kW, kH, 1.f / 255.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Center should be blue (texture blue * white diffuse = blue)
    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_LT(ctr[0], 0.1f) << "center R should be ~0 (blue texture)";
    EXPECT_LT(ctr[1], 0.1f) << "center G should be ~0";
    EXPECT_GT(ctr[2], 0.9f) << "center B should be ~1.0 (blue texture)";

    bb->Release();
    vb->Release();
    tex->Release();
}

// Triangle fan: center vertex at origin, 4 outer vertices → 2 triangles covering a quad.
// Uses FF path (XYZ + DIFFUSE), D3DPT_TRIANGLEFAN.
TEST_F(D3D9DrawGoldenTests, FFTriangle_TriangleFan)
{
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET,
                         D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING, FALSE), S_OK);

    // Fan center + 4 outer vertices (green), forming 3 triangles that tile the quad.
    struct V { float x, y, z; D3DCOLOR color; };
    const V verts[5] = {
        {  0.0f,  0.0f, 0.5f, 0xFF00FF00u },   // hub (green)
        { -0.8f, -0.8f, 0.5f, 0xFF00FF00u },   // outer 0
        {  0.8f, -0.8f, 0.5f, 0xFF00FF00u },   // outer 1  → tri: hub, 0, 1
        {  0.8f,  0.8f, 0.5f, 0xFF00FF00u },   // outer 2  → tri: hub, 1, 2
        { -0.8f,  0.8f, 0.5f, 0xFF00FF00u },   // outer 3  → tri: hub, 2, 3
    };

    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);
    void* m = nullptr;
    ASSERT_EQ(vb->Lock(0, sizeof(verts), &m, 0), S_OK);
    std::memcpy(m, verts, sizeof(verts));
    ASSERT_EQ(vb->Unlock(), S_OK);
    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(V)), S_OK);
    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE), S_OK);

    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 3), S_OK);  // 3 primitives = 3 tris
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    ASSERT_EQ(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb), S_OK);
    D3DLOCKED_RECT blr = {};
    ASSERT_EQ(bb->LockRect(&blr, nullptr, D3DLOCK_READONLY), S_OK);

    std::vector<float> pixels(kW * kH * 4);
    for (UINT y = 0; y < kH; ++y)
    {
        const uint8_t* row = static_cast<const uint8_t*>(blr.pBits) + y * blr.Pitch;
        for (UINT x = 0; x < kW; ++x)
        {
            UINT i = (y * kW + x) * 4;
            pixels[i+0] = row[x*4+2] / 255.f;
            pixels[i+1] = row[x*4+1] / 255.f;
            pixels[i+2] = row[x*4+0] / 255.f;
            pixels[i+3] = row[x*4+3] / 255.f;
        }
    }
    ASSERT_EQ(bb->UnlockRect(), S_OK);

    auto result = CheckGolden("d3d9_ff_fan_64x64", pixels.data(), kW, kH, 1.f / 255.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Center should be green (all triangles converge there)
    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_LT(ctr[0], 0.1f) << "center R should be ~0 (green)";
    EXPECT_GT(ctr[1], 0.9f) << "center G should be ~1.0 (green)";
    EXPECT_LT(ctr[2], 0.1f) << "center B should be ~0";

    bb->Release();
    vb->Release();
}

// Material source routing: D3DMCS_COLOR1 (default) means vertex diffuse supplies
// the material colour for lighting. With a white directional light from the front,
// a vertex colour of green should produce a green lit result.
TEST_F(D3D9DrawGoldenTests, FFTriangle_LightingColorVertexDiffuse)
{
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET,
                         D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE,   D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING,   TRUE), S_OK);

    // Material with zero diffuse — if routing works, vertex colour overrides it.
    D3DMATERIAL9 mat = {};
    mat.Diffuse  = { 0.f, 0.f, 0.f, 1.f };  // zero diffuse
    mat.Ambient  = { 0.f, 0.f, 0.f, 1.f };
    ASSERT_EQ(dev->SetMaterial(&mat), S_OK);

    // D3DMCS_COLOR1 = use vertex diffuse (the default)
    ASSERT_EQ(dev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_COLORVERTEX,           TRUE), S_OK);

    // White directional light from front (+Z direction in view space)
    D3DLIGHT9 light = {};
    light.Type      = D3DLIGHT_DIRECTIONAL;
    light.Diffuse   = { 1.f, 1.f, 1.f, 1.f };
    light.Direction = { 0.f, 0.f, -1.f };
    ASSERT_EQ(dev->SetLight(0, &light), S_OK);
    ASSERT_EQ(dev->LightEnable(0, TRUE), S_OK);

    // Vertices: XYZ + NORMAL + DIFFUSE colour green
    struct V { float x, y, z, nx, ny, nz; D3DCOLOR color; };
    const V verts[3] = {
        { -0.8f, -0.8f, 0.5f,  0.f, 0.f, 1.f,  0xFF00FF00u },
        {  0.0f,  0.8f, 0.5f,  0.f, 0.f, 1.f,  0xFF00FF00u },
        {  0.8f, -0.8f, 0.5f,  0.f, 0.f, 1.f,  0xFF00FF00u },
    };

    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);
    void* m = nullptr;
    ASSERT_EQ(vb->Lock(0, sizeof(verts), &m, 0), S_OK);
    std::memcpy(m, verts, sizeof(verts));
    ASSERT_EQ(vb->Unlock(), S_OK);
    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(V)), S_OK);
    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE), S_OK);
    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    ASSERT_EQ(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb), S_OK);
    D3DLOCKED_RECT blr = {};
    ASSERT_EQ(bb->LockRect(&blr, nullptr, D3DLOCK_READONLY), S_OK);

    std::vector<float> pixels(kW * kH * 4);
    for (UINT y = 0; y < kH; ++y)
    {
        const uint8_t* row = static_cast<const uint8_t*>(blr.pBits) + y * blr.Pitch;
        for (UINT x = 0; x < kW; ++x)
        {
            UINT i = (y * kW + x) * 4;
            pixels[i+0] = row[x*4+2] / 255.f;
            pixels[i+1] = row[x*4+1] / 255.f;
            pixels[i+2] = row[x*4+0] / 255.f;
            pixels[i+3] = row[x*4+3] / 255.f;
        }
    }
    ASSERT_EQ(bb->UnlockRect(), S_OK);

    auto result = CheckGolden("d3d9_ff_colorvertex_64x64", pixels.data(), kW, kH, 1.f / 255.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Center: vertex colour is green, light is white, NdotL = 1
    // → lit result should be fully green
    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_LT(ctr[0], 0.1f) << "R should be ~0 (no red in vertex colour)";
    EXPECT_GT(ctr[1], 0.9f) << "G should be ~1 (green vertex colour × white light)";
    EXPECT_LT(ctr[2], 0.1f) << "B should be ~0";

    bb->Release();
    vb->Release();
}

// Material source = MATERIAL: _material.Diffuse drives the lit colour,
// vertex colour is ignored even when present and COLORVERTEX is TRUE.
// Material diffuse = red; vertex colour = green; expect red output.
TEST_F(D3D9DrawGoldenTests, FFTriangle_LightingMaterialSourceMaterial)
{
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET,
                         D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE,              D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING,              TRUE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_COLORVERTEX,           TRUE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL), S_OK);

    D3DMATERIAL9 mat = {};
    mat.Diffuse = { 1.f, 0.f, 0.f, 1.f };  // red material
    ASSERT_EQ(dev->SetMaterial(&mat), S_OK);

    D3DLIGHT9 light = {};
    light.Type = D3DLIGHT_DIRECTIONAL; light.Diffuse = { 1.f, 1.f, 1.f, 1.f };
    light.Direction = { 0.f, 0.f, -1.f };
    ASSERT_EQ(dev->SetLight(0, &light), S_OK);
    ASSERT_EQ(dev->LightEnable(0, TRUE), S_OK);

    struct V { float x, y, z, nx, ny, nz; D3DCOLOR color; };
    const V verts[3] = {
        { -0.8f, -0.8f, 0.5f, 0.f, 0.f, 1.f, 0xFF00FF00u },  // green vertex colour
        {  0.0f,  0.8f, 0.5f, 0.f, 0.f, 1.f, 0xFF00FF00u },
        {  0.8f, -0.8f, 0.5f, 0.f, 0.f, 1.f, 0xFF00FF00u },
    };
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);
    void* m = nullptr; vb->Lock(0, sizeof(verts), &m, 0);
    std::memcpy(m, verts, sizeof(verts)); vb->Unlock();
    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(V)), S_OK);
    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE), S_OK);
    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
    D3DLOCKED_RECT blr = {};
    bb->LockRect(&blr, nullptr, D3DLOCK_READONLY);

    std::vector<float> pixels(kW * kH * 4);
    for (UINT y = 0; y < kH; ++y) {
        const uint8_t* row = static_cast<const uint8_t*>(blr.pBits) + y * blr.Pitch;
        for (UINT x = 0; x < kW; ++x) {
            UINT i = (y * kW + x) * 4;
            pixels[i+0] = row[x*4+2] / 255.f; pixels[i+1] = row[x*4+1] / 255.f;
            pixels[i+2] = row[x*4+0] / 255.f; pixels[i+3] = row[x*4+3] / 255.f;
        }
    }
    bb->UnlockRect();

    auto result = CheckGolden("d3d9_ff_mats_material_64x64", pixels.data(), kW, kH, 1.f / 255.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Material is red → lit colour must be red, not green
    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_GT(ctr[0], 0.9f) << "R should be ~1 (red material diffuse)";
    EXPECT_LT(ctr[1], 0.1f) << "G should be ~0 (vertex green ignored)";

    bb->Release(); vb->Release();
}

// D3DRS_COLORVERTEX = FALSE: all material sources forced to MATERIAL regardless.
// Material diffuse = blue; vertex colour = red → expect blue output.
TEST_F(D3D9DrawGoldenTests, FFTriangle_LightingColorVertexFalse)
{
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET,
                         D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE,    D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING,    TRUE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_COLORVERTEX, FALSE), S_OK);
    // Source is D3DMCS_COLOR1 but COLORVERTEX=FALSE overrides it
    ASSERT_EQ(dev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1), S_OK);

    D3DMATERIAL9 mat = {};
    mat.Diffuse = { 0.f, 0.f, 1.f, 1.f };  // blue material
    ASSERT_EQ(dev->SetMaterial(&mat), S_OK);

    D3DLIGHT9 light = {};
    light.Type = D3DLIGHT_DIRECTIONAL; light.Diffuse = { 1.f, 1.f, 1.f, 1.f };
    light.Direction = { 0.f, 0.f, -1.f };
    ASSERT_EQ(dev->SetLight(0, &light), S_OK);
    ASSERT_EQ(dev->LightEnable(0, TRUE), S_OK);

    struct V { float x, y, z, nx, ny, nz; D3DCOLOR color; };
    const V verts[3] = {
        { -0.8f, -0.8f, 0.5f, 0.f, 0.f, 1.f, 0xFFFF0000u },  // red vertex colour
        {  0.0f,  0.8f, 0.5f, 0.f, 0.f, 1.f, 0xFFFF0000u },
        {  0.8f, -0.8f, 0.5f, 0.f, 0.f, 1.f, 0xFFFF0000u },
    };
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);
    void* m = nullptr; vb->Lock(0, sizeof(verts), &m, 0);
    std::memcpy(m, verts, sizeof(verts)); vb->Unlock();
    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(V)), S_OK);
    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE), S_OK);
    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
    D3DLOCKED_RECT blr = {};
    bb->LockRect(&blr, nullptr, D3DLOCK_READONLY);

    std::vector<float> pixels(kW * kH * 4);
    for (UINT y = 0; y < kH; ++y) {
        const uint8_t* row = static_cast<const uint8_t*>(blr.pBits) + y * blr.Pitch;
        for (UINT x = 0; x < kW; ++x) {
            UINT i = (y * kW + x) * 4;
            pixels[i+0] = row[x*4+2] / 255.f; pixels[i+1] = row[x*4+1] / 255.f;
            pixels[i+2] = row[x*4+0] / 255.f; pixels[i+3] = row[x*4+3] / 255.f;
        }
    }
    bb->UnlockRect();

    auto result = CheckGolden("d3d9_ff_colorvertex_false_64x64", pixels.data(), kW, kH, 1.f / 255.f);
    EXPECT_TRUE(result.passed) << result.message;

    // COLORVERTEX=FALSE → material blue, not vertex red
    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_LT(ctr[0], 0.1f) << "R should be ~0 (red vertex ignored)";
    EXPECT_GT(ctr[2], 0.9f) << "B should be ~1 (blue material used)";

    bb->Release(); vb->Release();
}

// Emissive material source = D3DMCS_COLOR1: vertex colour adds as emissive.
// Emissive is additive, independent of lighting. With black material diffuse
// and a green vertex emissive, the result should be green regardless of NdotL.
TEST_F(D3D9DrawGoldenTests, FFTriangle_EmissiveFromVertexColor)
{
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET,
                         D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE,               D3DCULL_NONE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_LIGHTING,               TRUE), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_COLORVERTEX,            TRUE), S_OK);
    // All material properties from _material (black) except emissive from vertex
    ASSERT_EQ(dev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,  D3DMCS_MATERIAL), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1), S_OK);

    D3DMATERIAL9 mat = {};  // all zero: no diffuse, no ambient, no emissive
    ASSERT_EQ(dev->SetMaterial(&mat), S_OK);

    D3DLIGHT9 light = {};
    light.Type = D3DLIGHT_DIRECTIONAL; light.Diffuse = { 1.f, 1.f, 1.f, 1.f };
    light.Direction = { 0.f, 0.f, -1.f };
    ASSERT_EQ(dev->SetLight(0, &light), S_OK);
    ASSERT_EQ(dev->LightEnable(0, TRUE), S_OK);

    struct V { float x, y, z, nx, ny, nz; D3DCOLOR color; };
    const V verts[3] = {
        { -0.8f, -0.8f, 0.5f, 0.f, 0.f, 1.f, 0xFF00FF00u },  // green emissive
        {  0.0f,  0.8f, 0.5f, 0.f, 0.f, 1.f, 0xFF00FF00u },
        {  0.8f, -0.8f, 0.5f, 0.f, 0.f, 1.f, 0xFF00FF00u },
    };
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);
    void* m = nullptr; vb->Lock(0, sizeof(verts), &m, 0);
    std::memcpy(m, verts, sizeof(verts)); vb->Unlock();
    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(V)), S_OK);
    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE), S_OK);
    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    ASSERT_EQ(dev->SetPixelShader(nullptr), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
    D3DLOCKED_RECT blr = {};
    bb->LockRect(&blr, nullptr, D3DLOCK_READONLY);

    std::vector<float> pixels(kW * kH * 4);
    for (UINT y = 0; y < kH; ++y) {
        const uint8_t* row = static_cast<const uint8_t*>(blr.pBits) + y * blr.Pitch;
        for (UINT x = 0; x < kW; ++x) {
            UINT i = (y * kW + x) * 4;
            pixels[i+0] = row[x*4+2] / 255.f; pixels[i+1] = row[x*4+1] / 255.f;
            pixels[i+2] = row[x*4+0] / 255.f; pixels[i+3] = row[x*4+3] / 255.f;
        }
    }
    bb->UnlockRect();

    auto result = CheckGolden("d3d9_ff_emissive_vertex_64x64", pixels.data(), kW, kH, 1.f / 255.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Emissive = green vertex colour; diffuse contribution = 0 (black material)
    // → result should be green
    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_LT(ctr[0], 0.1f) << "R should be ~0";
    EXPECT_GT(ctr[1], 0.9f) << "G should be ~1 (emissive from vertex colour)";
    EXPECT_LT(ctr[2], 0.1f) << "B should be ~0";

    bb->Release(); vb->Release();
}
