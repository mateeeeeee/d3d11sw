#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

#include "golden_test_util.h"
#include "d3d9/resources/texture.h"
#include "core/shaders/shader_abi.h"

using namespace d3dsw;

namespace
{
    constexpr UINT kW = 32;
    constexpr UINT kH = 32;

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

// Draws a triangle textured with a 4×4 checkerboard (red+blue).
// Center pixel maps to UV (0.5, 0.5) which lands on a red texel — verifies
// the full SM3 TEXLD path: texture upload → SRV wiring → sampler → sample.
TEST(D3D9Texture, SampleCheckerboard_CenterIsRed)
{
    auto vsbc = ReadBytecode(D3D9SW_SHADER_DIR "/vs_texture.o");
    auto psbc = ReadBytecode(D3D9SW_SHADER_DIR "/ps_texture.o");
    if (vsbc.empty() || psbc.empty())
    {
        GTEST_SKIP() << "FXC-compiled .o files missing — run tests/d3d9/shaders/compile.sh";
    }

    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);

    // 4×4 checkerboard: red (0xFF0000FF in A8R8G8B8) and blue (0xFFFF0000).
    // A8R8G8B8 = ARGB, stored in memory as B G R A (BGRA byte order).
    constexpr UINT texW = 4, texH = 4;
    uint32_t texels[texH][texW];
    for (UINT y = 0; y < texH; ++y)
    {
        for (UINT x = 0; x < texW; ++x)
        {
            bool redCell = ((x + y) % 2 == 0);
            // A8R8G8B8 value: red=0xFFFF0000, blue=0xFF0000FF
            texels[y][x] = redCell ? 0xFFFF0000u : 0xFF0000FFu;
        }
    }

    IDirect3DTexture9* tex = nullptr;
    ASSERT_EQ(dev->CreateTexture(texW, texH, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, nullptr), S_OK);
    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(tex->LockRect(0, &lr, nullptr, 0), S_OK);
    for (UINT y = 0; y < texH; ++y)
    {
        std::memcpy(static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch, texels[y], texW * 4);
    }
    ASSERT_EQ(tex->UnlockRect(0), S_OK);
    ASSERT_EQ(dev->SetTexture(0, tex), S_OK);
    ASSERT_EQ(dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT), S_OK);
    ASSERT_EQ(dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT), S_OK);
    ASSERT_EQ(dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE),  S_OK);

    // CW-wound triangle covering the full viewport with UV [0,1]×[0,1].
    struct V { float x, y, z, u, v; };
    const V verts[3] = {
        { -1.f, -1.f, 0.5f,  0.f, 1.f },   // lower-left  → UV (0, 1)
        {  0.f,  1.f, 0.5f,  0.5f, 0.f },  // top-center  → UV (0.5, 0)
        {  1.f, -1.f, 0.5f,  1.f, 1.f },   // lower-right → UV (1, 1)
    };
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);
    void* m = nullptr;
    ASSERT_EQ(vb->Lock(0, sizeof(verts), &m, 0), S_OK);
    std::memcpy(m, verts, sizeof(verts));
    ASSERT_EQ(vb->Unlock(), S_OK);
    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(V)), S_OK);

    const D3DVERTEXELEMENT9 elems[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END(),
    };
    IDirect3DVertexDeclaration9* decl = nullptr;
    ASSERT_EQ(dev->CreateVertexDeclaration(elems, &decl), S_OK);
    ASSERT_EQ(dev->SetVertexDeclaration(decl), S_OK);

    IDirect3DVertexShader9* vs = nullptr;
    ASSERT_EQ(dev->CreateVertexShader(reinterpret_cast<const DWORD*>(vsbc.data()), &vs), S_OK);
    ASSERT_EQ(dev->SetVertexShader(vs), S_OK);

    IDirect3DPixelShader9* ps = nullptr;
    ASSERT_EQ(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(psbc.data()), &ps), S_OK);
    ASSERT_EQ(dev->SetPixelShader(ps), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    ASSERT_EQ(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb), S_OK);
    D3DLOCKED_RECT blr = {};
    ASSERT_EQ(bb->LockRect(&blr, nullptr, D3DLOCK_READONLY), S_OK);

    // Center pixel UV ≈ (0.5, 0.5). Checkerboard (0+0)%2==0 at texel (0,0),
    // (1,1)%2==0 also red, (1,0)%2==1 blue, etc. At UV (0.5,0.5) with a 4×4
    // texture we land at texel (2,2): (2+2)%2==0 → red.
    // BGRA memory order: B=0x00, G=0x00, R=0xFF, A=0xFF for A8R8G8B8 red.
    const uint8_t* ctr = static_cast<const uint8_t*>(blr.pBits)
                       + (kH / 2) * blr.Pitch + (kW / 2) * 4;
    EXPECT_EQ(ctr[0], 0x00) << "center B should be 0 (red texel)";
    EXPECT_EQ(ctr[1], 0x00) << "center G should be 0 (red texel)";
    EXPECT_EQ(ctr[2], 0xFF) << "center R should be 0xFF (red texel)";

    ASSERT_EQ(bb->UnlockRect(), S_OK);
    bb->Release();
    ps->Release(); vs->Release(); decl->Release(); vb->Release(); tex->Release();
    dev->Release();
}

// D3DFMT_R8G8B8 (24bpp BGR) texture creation and LockRect.
// Verifies: pitch = width*3, GetDesc returns the original format, and
// data written to the 24bpp shadow buffer can be read back.
TEST(D3D9Texture, R8G8B8_LockPitchAndReadback)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    constexpr UINT texW = 4, texH = 4;
    IDirect3DTexture9* tex = nullptr;
    ASSERT_EQ(dev->CreateTexture(texW, texH, 1, 0, D3DFMT_R8G8B8, D3DPOOL_DEFAULT, &tex, nullptr), S_OK);
    ASSERT_NE(tex, nullptr);

    // GetLevelDesc must report the original D3DFMT_R8G8B8.
    D3DSURFACE_DESC desc = {};
    ASSERT_EQ(tex->GetLevelDesc(0, &desc), S_OK);
    EXPECT_EQ(desc.Format, D3DFMT_R8G8B8);

    // First lock: pitch must be width * 3 (24bpp).
    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(tex->LockRect(0, &lr, nullptr, 0), S_OK);
    EXPECT_EQ(lr.Pitch, static_cast<INT>(texW * 3)) << "R8G8B8 pitch must be width*3";

    // Write pixels in BGR order: pixel (0,0) = pure red (B=0,G=0,R=255).
    uint8_t* row0 = static_cast<uint8_t*>(lr.pBits);
    row0[0] = 0;   row0[1] = 0;   row0[2] = 255;  // pixel (0,0): B=0 G=0 R=255
    row0[3] = 0;   row0[4] = 255; row0[5] = 0;    // pixel (1,0): B=0 G=255 R=0

    ASSERT_EQ(tex->UnlockRect(0), S_OK);

    // Lock again — shadow buffer persists, same data is readable.
    D3DLOCKED_RECT lr2 = {};
    ASSERT_EQ(tex->LockRect(0, &lr2, nullptr, 0), S_OK);
    const uint8_t* rb = static_cast<const uint8_t*>(lr2.pBits);
    EXPECT_EQ(rb[0], 0)   << "B of (0,0) should be 0";
    EXPECT_EQ(rb[1], 0)   << "G of (0,0) should be 0";
    EXPECT_EQ(rb[2], 255) << "R of (0,0) should be 255 (red)";
    EXPECT_EQ(rb[3], 0)   << "B of (1,0) should be 0";
    EXPECT_EQ(rb[4], 255) << "G of (1,0) should be 255 (green)";
    EXPECT_EQ(rb[5], 0)   << "R of (1,0) should be 0";
    ASSERT_EQ(tex->UnlockRect(0), S_OK);

    tex->Release();
    dev->Release();
}

// L8 FillSRV must set swizzle (R,R,R,1) so luminance broadcasts to all RGB channels.
TEST(D3D9Texture, L8_FillSRV_SetsLuminanceSwizzle)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    IDirect3DTexture9* tex = nullptr;
    ASSERT_EQ(dev->CreateTexture(4, 4, 1, 0, D3DFMT_L8, D3DPOOL_DEFAULT, &tex, nullptr), S_OK);
    ASSERT_NE(tex, nullptr);

    // Write luminance values
    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(tex->LockRect(0, &lr, nullptr, 0), S_OK);
    for (UINT y = 0; y < 4; ++y)
    {
        uint8_t* row = static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch;
        for (UINT x = 0; x < 4; ++x) { row[x] = 200; }
    }
    ASSERT_EQ(tex->UnlockRect(0), S_OK);

    // FillSRV should set hasSwizzle=1 with (0,0,0,5) mapping (R,R,R,1)
    SW_SRV srv = {};
    static_cast<d3dsw::D3D9TextureSW*>(tex)->FillSRV(srv);
    EXPECT_NE(srv.hasSwizzle, 0) << "L8 texture must have swizzle active";
    EXPECT_EQ(srv.swizzle[0], 0) << "R channel should map to R (luminance)";
    EXPECT_EQ(srv.swizzle[1], 0) << "G channel should map to R (luminance)";
    EXPECT_EQ(srv.swizzle[2], 0) << "B channel should map to R (luminance)";
    EXPECT_EQ(srv.swizzle[3], 5) << "A channel should map to constant 1.0";

    tex->Release();
    dev->Release();
}

// A8 FillSRV must swizzle (0,0,0,R) so alpha appears only in .w.
TEST(D3D9Texture, A8_FillSRV_AlphaSwizzle)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    IDirect3DTexture9* tex = nullptr;
    ASSERT_EQ(dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8, D3DPOOL_DEFAULT, &tex, nullptr), S_OK);
    ASSERT_NE(tex, nullptr);

    SW_SRV srv = {};
    static_cast<d3dsw::D3D9TextureSW*>(tex)->FillSRV(srv);
    EXPECT_NE(srv.hasSwizzle, 0);
    EXPECT_EQ(srv.swizzle[0], 4) << "R should be constant 0";
    EXPECT_EQ(srv.swizzle[1], 4) << "G should be constant 0";
    EXPECT_EQ(srv.swizzle[2], 4) << "B should be constant 0";
    EXPECT_EQ(srv.swizzle[3], 0) << "A should be R component (the stored alpha)";

    tex->Release();
    dev->Release();
}
