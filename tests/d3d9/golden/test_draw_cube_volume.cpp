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
                px[i+0] = row[x*4+2] / 255.f;  // R
                px[i+1] = row[x*4+1] / 255.f;  // G
                px[i+2] = row[x*4+0] / 255.f;  // B
                px[i+3] = row[x*4+3] / 255.f;  // A
            }
        }
        bb->UnlockRect();
        bb->Release();
        return px;
    }

    // Fill a cube face mip 0 with a solid D3DCOLOR.
    void FillCubeFace(IDirect3DCubeTexture9* tex, D3DCUBEMAP_FACES face, D3DCOLOR color, UINT size)
    {
        D3DLOCKED_RECT lr = {};
        tex->LockRect(face, 0, &lr, nullptr, 0);
        for (UINT y = 0; y < size; ++y)
        {
            auto* row = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch);
            for (UINT x = 0; x < size; ++x) { row[x] = color; }
        }
        tex->UnlockRect(face, 0);
    }

    // Fill a volume slice with a solid D3DCOLOR.
    void FillVolumeSlice(IDirect3DVolumeTexture9* vol, UINT slice, D3DCOLOR color, UINT w, UINT h)
    {
        D3DLOCKED_BOX lb = {};
        D3DBOX box = { 0, 0, w, h, slice, slice + 1 };  // Left,Top,Right,Bottom,Front,Back
        vol->LockBox(0, &lb, &box, 0);
        for (UINT y = 0; y < h; ++y)
        {
            auto* row = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(lb.pBits) + y * lb.RowPitch);
            for (UINT x = 0; x < w; ++x) { row[x] = color; }
        }
        vol->UnlockBox(0);
    }

    IDirect3DVertexShader9* LoadVS(IDirect3DDevice9* dev, const char* path)
    {
        auto bc = ReadBytecode(path);
        if (bc.empty()) { return nullptr; }
        IDirect3DVertexShader9* vs = nullptr;
        dev->CreateVertexShader(reinterpret_cast<const DWORD*>(bc.data()), &vs);
        return vs;
    }

    IDirect3DPixelShader9* LoadPS(IDirect3DDevice9* dev, const char* path)
    {
        auto bc = ReadBytecode(path);
        if (bc.empty()) { return nullptr; }
        IDirect3DPixelShader9* ps = nullptr;
        dev->CreatePixelShader(reinterpret_cast<const DWORD*>(bc.data()), &ps);
        return ps;
    }
}

struct D3D9TextureGoldenTests : ::testing::Test {};

// ---------------------------------------------------------------------------
// Cubemap
// ---------------------------------------------------------------------------

// Samples the +X face of a cube texture by drawing a full-viewport triangle
// with direction vectors pointing to (1, 0, 0).  The +X face is filled red;
// all other faces are black.  Every pixel in the output should be red.
TEST_F(D3D9TextureGoldenTests, CubemapSample_PositiveXFace)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    auto* vs = LoadVS(dev, D3D9SW_SHADER_DIR "/vs_tc3.o");
    auto* ps = LoadPS(dev, D3D9SW_SHADER_DIR "/ps_cubemap.o");
    ASSERT_NE(vs, nullptr) << "vs_tc3.o not found — run tests/d3d9/shaders/compile.sh";
    ASSERT_NE(ps, nullptr) << "ps_cubemap.o not found — run tests/d3d9/shaders/compile.sh";

    constexpr UINT kSize = 4;
    IDirect3DCubeTexture9* cube = nullptr;
    ASSERT_EQ(dev->CreateCubeTexture(kSize, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &cube, nullptr), S_OK);

    FillCubeFace(cube, D3DCUBEMAP_FACE_POSITIVE_X, D3DCOLOR_ARGB(255, 255,   0,   0), kSize);  // red
    FillCubeFace(cube, D3DCUBEMAP_FACE_NEGATIVE_X, D3DCOLOR_ARGB(255,   0, 255,   0), kSize);  // green
    FillCubeFace(cube, D3DCUBEMAP_FACE_POSITIVE_Y, D3DCOLOR_ARGB(255,   0,   0, 255), kSize);  // blue
    FillCubeFace(cube, D3DCUBEMAP_FACE_NEGATIVE_Y, D3DCOLOR_ARGB(255, 255, 255,   0), kSize);  // yellow
    FillCubeFace(cube, D3DCUBEMAP_FACE_POSITIVE_Z, D3DCOLOR_ARGB(255, 255,   0, 255), kSize);  // magenta
    FillCubeFace(cube, D3DCUBEMAP_FACE_NEGATIVE_Z, D3DCOLOR_ARGB(255,   0, 255, 255), kSize);  // cyan

    dev->SetTexture(0, cube);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

    struct V { float x, y, z, tx, ty, tz, tw; };
    // Full-viewport CW triangle; all direction vectors point to +X face.
    const V verts[3] = {
        { -1.f, -1.f, 0.5f,  1.f, -1.f, -1.f, 0.f },
        { -1.f,  3.f, 0.5f,  1.f,  1.f, -1.f, 0.f },
        {  3.f, -1.f, 0.5f,  1.f, -1.f,  1.f, 0.f },
    };
    IDirect3DVertexBuffer9* vb = nullptr;
    dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr);
    void* m = nullptr; vb->Lock(0, sizeof(verts), &m, 0); std::memcpy(m, verts, sizeof(verts)); vb->Unlock();
    dev->SetStreamSource(0, vb, 0, sizeof(V));

    const D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END(),
    };
    IDirect3DVertexDeclaration9* vdecl = nullptr;
    dev->CreateVertexDeclaration(decl, &vdecl);
    dev->SetVertexDeclaration(vdecl);
    dev->SetVertexShader(vs);
    dev->SetPixelShader(ps);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF, 0, 0, 0), 1.f, 0);
    dev->BeginScene();
    dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
    dev->EndScene();

    auto pixels = Readback(dev);
    auto result = CheckGolden("d3d9_cubemap_positive_x_64x64", pixels.data(), kW, kH, 1.f/255.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Centre pixel should be red.
    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_GT(ctr[0], 0.9f) << "R should be ~1 (red face)";
    EXPECT_LT(ctr[1], 0.1f) << "G should be ~0";
    EXPECT_LT(ctr[2], 0.1f) << "B should be ~0";

    cube->Release(); vb->Release(); vdecl->Release(); vs->Release(); ps->Release();
    dev->Release();
}

// ---------------------------------------------------------------------------
// Volume texture
// ---------------------------------------------------------------------------

// Samples a volume texture, drawing two full-viewport passes (one per W slice)
// split via scissor rect — left half gets slice 0 (red), right half slice 2 (blue).
TEST_F(D3D9TextureGoldenTests, VolumeSample_TwoSlices)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    auto* vs = LoadVS(dev, D3D9SW_SHADER_DIR "/vs_tc3.o");
    auto* ps = LoadPS(dev, D3D9SW_SHADER_DIR "/ps_volume.o");
    ASSERT_NE(vs, nullptr) << "vs_tc3.o not found — run tests/d3d9/shaders/compile.sh";
    ASSERT_NE(ps, nullptr) << "ps_volume.o not found — run tests/d3d9/shaders/compile.sh";

    constexpr UINT kSz = 4;
    IDirect3DVolumeTexture9* vol = nullptr;
    ASSERT_EQ(dev->CreateVolumeTexture(kSz, kSz, kSz, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &vol, nullptr), S_OK);

    FillVolumeSlice(vol, 0, D3DCOLOR_ARGB(255, 255,   0,   0), kSz, kSz);  // red
    FillVolumeSlice(vol, 1, D3DCOLOR_ARGB(255,   0, 255,   0), kSz, kSz);  // green
    FillVolumeSlice(vol, 2, D3DCOLOR_ARGB(255,   0,   0, 255), kSz, kSz);  // blue
    FillVolumeSlice(vol, 3, D3DCOLOR_ARGB(255, 255, 255, 255), kSz, kSz);  // white

    dev->SetTexture(0, vol);
    dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

    const D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END(),
    };
    IDirect3DVertexDeclaration9* vdecl = nullptr;
    dev->CreateVertexDeclaration(decl, &vdecl);
    dev->SetVertexDeclaration(vdecl);
    dev->SetVertexShader(vs);
    dev->SetPixelShader(ps);

    // Fullscreen CW triangle large enough to cover entire viewport regardless of scissor.
    // Vertex layout: POSITION(3) + TEXCOORD0(4) = 28 bytes
    struct V { float x, y, z, tu, tv, tw, ta; };

    auto drawFull = [&](float tw_val)
    {
        const V tri[3] = {
            { -1.f, -1.f, 0.5f,  0.5f, 0.5f, tw_val, 0.f },
            { -1.f,  3.f, 0.5f,  0.5f, 0.5f, tw_val, 0.f },
            {  3.f, -1.f, 0.5f,  0.5f, 0.5f, tw_val, 0.f },
        };
        IDirect3DVertexBuffer9* vb = nullptr;
        dev->CreateVertexBuffer(sizeof(tri), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr);
        void* m = nullptr; vb->Lock(0, sizeof(tri), &m, 0); std::memcpy(m, tri, sizeof(tri)); vb->Unlock();
        dev->SetStreamSource(0, vb, 0, sizeof(V));
        dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
        vb->Release();
    };

    dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF, 0, 0, 0), 1.f, 0);
    dev->BeginScene();

    // Left half (x pixels 0..31): sample slice 0 (w=0.125) → red
    RECT lScissor = { 0, 0, static_cast<LONG>(kW / 2), static_cast<LONG>(kH) };
    dev->SetScissorRect(&lScissor);
    drawFull(0.125f);

    // Right half (x pixels 32..63): sample slice 2 (w=0.625) → blue
    RECT rScissor = { static_cast<LONG>(kW / 2), 0, static_cast<LONG>(kW), static_cast<LONG>(kH) };
    dev->SetScissorRect(&rScissor);
    drawFull(0.625f);

    dev->EndScene();

    auto pixels = Readback(dev);
    auto result = CheckGolden("d3d9_volume_two_slices_64x64", pixels.data(), kW, kH, 1.f/255.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Left-centre pixel: red (slice 0).
    const float* lp = pixels.data() + (kH/2 * kW + kW/4) * 4;
    EXPECT_GT(lp[0], 0.9f) << "Left R should be ~1";
    EXPECT_LT(lp[1], 0.1f) << "Left G should be ~0";
    EXPECT_LT(lp[2], 0.1f) << "Left B should be ~0";

    // Right-centre pixel: blue (slice 2).
    const float* rp = pixels.data() + (kH/2 * kW + kW*3/4) * 4;
    EXPECT_LT(rp[0], 0.1f) << "Right R should be ~0";
    EXPECT_LT(rp[1], 0.1f) << "Right G should be ~0";
    EXPECT_GT(rp[2], 0.9f) << "Right B should be ~1";

    vol->Release(); vdecl->Release(); vs->Release(); ps->Release();
    dev->Release();
}
