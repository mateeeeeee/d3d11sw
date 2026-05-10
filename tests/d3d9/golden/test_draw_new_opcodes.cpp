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

    // Upload a 4×4 red/blue checkerboard to a D3DFMT_A8R8G8B8 texture and bind it.
    IDirect3DTexture9* UploadCheckerboard(IDirect3DDevice9* dev)
    {
        constexpr UINT texW = 4, texH = 4;
        uint32_t texels[texH][texW];
        for (UINT y = 0; y < texH; ++y)
            for (UINT x = 0; x < texW; ++x)
                texels[y][x] = ((x + y) % 2 == 0) ? 0xFFFF0000u : 0xFF0000FFu;

        IDirect3DTexture9* tex = nullptr;
        dev->CreateTexture(texW, texH, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, nullptr);
        D3DLOCKED_RECT lr = {};
        tex->LockRect(0, &lr, nullptr, 0);
        for (UINT y = 0; y < texH; ++y)
            std::memcpy(static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch, texels[y], texW * 4);
        tex->UnlockRect(0);
        return tex;
    }
}

struct D3D9DrawGoldenTests : ::testing::Test
{
    IDirect3DDevice9* dev = nullptr;
    void SetUp() override   { dev = MakeHeadless(); ASSERT_NE(dev, nullptr); }
    void TearDown() override { if (dev) { dev->Release(); dev = nullptr; } }
};

// TEXLDL: tex2Dlod with explicit LOD=0 should produce the same checkerboard
// as regular sampling. The golden locks the TEXLDL code path end-to-end.
TEST_F(D3D9DrawGoldenTests, OpcodeTEXLDL_ExplicitLOD)
{
    auto vsbc = ReadBytecode(D3D9SW_SHADER_DIR "/vs_texture.o");
    auto psbc = ReadBytecode(D3D9SW_SHADER_DIR "/ps_texldl.o");
    ASSERT_FALSE(vsbc.empty()) << "vs_texture.o not found — run compile.sh";
    ASSERT_FALSE(psbc.empty()) << "ps_texldl.o not found — run compile.sh";

    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);

    IDirect3DTexture9* tex = UploadCheckerboard(dev);
    ASSERT_EQ(dev->SetTexture(0, tex), S_OK);
    ASSERT_EQ(dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT), S_OK);
    ASSERT_EQ(dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT), S_OK);
    ASSERT_EQ(dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE),  S_OK);

    // Explicit LOD = 0 via c0.x
    const float lod[4] = { 0.f, 0.f, 0.f, 0.f };
    ASSERT_EQ(dev->SetPixelShaderConstantF(0, lod, 1), S_OK);

    struct V { float x, y, z, u, v; };
    const V verts[3] = {
        { -1.f, -1.f, 0.5f,  0.f, 1.f },
        {  0.f,  1.f, 0.5f,  0.5f, 0.f },
        {  1.f, -1.f, 0.5f,  1.f, 1.f },
    };
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts),0,0,D3DPOOL_DEFAULT,&vb,nullptr), S_OK);
    void* m = nullptr; vb->Lock(0,sizeof(verts),&m,0); std::memcpy(m,verts,sizeof(verts)); vb->Unlock();
    ASSERT_EQ(dev->SetStreamSource(0,vb,0,sizeof(V)), S_OK);

    const D3DVERTEXELEMENT9 elems[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0,12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END(),
    };
    IDirect3DVertexDeclaration9* decl = nullptr;
    ASSERT_EQ(dev->CreateVertexDeclaration(elems,&decl), S_OK);
    ASSERT_EQ(dev->SetVertexDeclaration(decl), S_OK);

    IDirect3DVertexShader9* vs = nullptr;
    ASSERT_EQ(dev->CreateVertexShader(reinterpret_cast<const DWORD*>(vsbc.data()),&vs), S_OK);
    ASSERT_EQ(dev->SetVertexShader(vs), S_OK);

    IDirect3DPixelShader9* ps = nullptr;
    ASSERT_EQ(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(psbc.data()),&ps), S_OK);
    ASSERT_EQ(dev->SetPixelShader(ps), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST,0,1), S_OK);
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

    auto result = CheckGolden("d3d9_opcode_texldl_64x64", pixels.data(), kW, kH, 0.f);
    EXPECT_TRUE(result.passed) << result.message;

    bb->Release();
    if (ps) ps->Release(); if (vs) vs->Release();
    decl->Release(); vb->Release(); tex->Release();
}

// REP/ENDREP: PS accumulates g_step N times via rep loop.
// With g_step=(0.1,0,0,0) and g_count=3: R output = 0.3.
TEST_F(D3D9DrawGoldenTests, OpcodeREP_ENDREP_Accumulates)
{
    auto vsbc = ReadBytecode(D3D9SW_SHADER_DIR "/vs_passthrough.o");
    auto psbc = ReadBytecode(D3D9SW_SHADER_DIR "/ps_rep.o");
    ASSERT_FALSE(vsbc.empty()) << "vs_passthrough.o not found — run compile.sh";
    if (psbc.empty())
    {
        GTEST_SKIP() << "ps_rep.o not found or shader uses unsupported features — run compile.sh";
    }

    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);

    // step = 0.1 per iteration, 3 literal iterations → R = 0.3
    const float step[4] = { 0.1f, 0.f, 0.f, 1.f };
    ASSERT_EQ(dev->SetPixelShaderConstantF(0, step, 1), S_OK);

    struct V { float x, y, z; };
    const V verts[3] = { {-1.f,-1.f,0.5f}, {0.f,1.f,0.5f}, {1.f,-1.f,0.5f} };
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts),0,0,D3DPOOL_DEFAULT,&vb,nullptr), S_OK);
    void* m = nullptr; vb->Lock(0,sizeof(verts),&m,0); std::memcpy(m,verts,sizeof(verts)); vb->Unlock();
    ASSERT_EQ(dev->SetStreamSource(0,vb,0,sizeof(V)), S_OK);

    const D3DVERTEXELEMENT9 elems[] = {
        {0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},D3DDECL_END()
    };
    IDirect3DVertexDeclaration9* decl = nullptr;
    ASSERT_EQ(dev->CreateVertexDeclaration(elems,&decl), S_OK);
    ASSERT_EQ(dev->SetVertexDeclaration(decl), S_OK);

    IDirect3DVertexShader9* vs = nullptr;
    ASSERT_EQ(dev->CreateVertexShader(reinterpret_cast<const DWORD*>(vsbc.data()),&vs), S_OK);
    ASSERT_EQ(dev->SetVertexShader(vs), S_OK);

    IDirect3DPixelShader9* ps = nullptr;
    ASSERT_EQ(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(psbc.data()),&ps), S_OK);
    ASSERT_NE(ps, nullptr) << "ps_rep.o failed to parse";
    ASSERT_EQ(dev->SetPixelShader(ps), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST,0,1), S_OK);
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

    auto result = CheckGolden("d3d9_opcode_rep_64x64", pixels.data(), kW, kH, 0.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Sanity: center R ≈ 0.3 (3 × 0.1, quantized to 8-bit ≈ 77/255)
    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_NEAR(ctr[0], 0.3f, 2.f / 255.f) << "center R should be 3×0.1=0.3";

    bb->Release();
    if (ps) ps->Release(); vs->Release(); decl->Release(); vb->Release();
}
