#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <cstdint>
#include <cstring>
#include <cmath>
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

    // Draw a CW-wound full-viewport triangle with vs_passthrough + a custom PS,
    // set one vec4 constant in c0, readback and return the float RGBA pixel array.
    std::vector<float> DrawSolid(IDirect3DDevice9* dev, const char* psPath, const float c0[4])
    {
        auto vsbc = ReadBytecode(D3D9SW_SHADER_DIR "/vs_passthrough.o");
        auto psbc = ReadBytecode(psPath);
        if (vsbc.empty() || psbc.empty()) { return {}; }

        dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0);

        struct V { float x, y, z; };
        const V verts[3] = { {-1.f,-1.f,0.5f}, {0.f,1.f,0.5f}, {1.f,-1.f,0.5f} };
        IDirect3DVertexBuffer9* vb = nullptr;
        dev->CreateVertexBuffer(sizeof(verts),0,0,D3DPOOL_DEFAULT,&vb,nullptr);
        void* m = nullptr; vb->Lock(0,sizeof(verts),&m,0); std::memcpy(m,verts,sizeof(verts)); vb->Unlock();
        dev->SetStreamSource(0,vb,0,sizeof(V));

        const D3DVERTEXELEMENT9 elems[] = {
            { 0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0 },D3DDECL_END()
        };
        IDirect3DVertexDeclaration9* decl = nullptr;
        dev->CreateVertexDeclaration(elems, &decl);
        dev->SetVertexDeclaration(decl);

        IDirect3DVertexShader9* vs = nullptr;
        dev->CreateVertexShader(reinterpret_cast<const DWORD*>(vsbc.data()), &vs);
        dev->SetVertexShader(vs);

        IDirect3DPixelShader9* ps = nullptr;
        dev->CreatePixelShader(reinterpret_cast<const DWORD*>(psbc.data()), &ps);
        dev->SetPixelShader(ps);
        dev->SetPixelShaderConstantF(0, c0, 1);

        dev->BeginScene();
        dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
        dev->EndScene();

        IDirect3DSurface9* bb = nullptr;
        dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bb);
        D3DLOCKED_RECT lr = {};
        bb->LockRect(&lr, nullptr, D3DLOCK_READONLY);

        std::vector<float> pixels(kW * kH * 4);
        for (UINT y = 0; y < kH; ++y)
        {
            const uint8_t* row = static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch;
            for (UINT x = 0; x < kW; ++x)
            {
                UINT i = (y * kW + x) * 4;
                pixels[i+0] = row[x*4+2] / 255.f; // R
                pixels[i+1] = row[x*4+1] / 255.f; // G
                pixels[i+2] = row[x*4+0] / 255.f; // B
                pixels[i+3] = row[x*4+3] / 255.f; // A
            }
        }
        bb->UnlockRect();

        bb->Release();
        if (ps)    { ps->Release(); }
        if (vs)    { vs->Release(); }
        if (decl)  { decl->Release(); }
        if (vb)    { vb->Release(); }
        return pixels;
    }
}

struct D3D9DrawGoldenTests : ::testing::Test
{
    IDirect3DDevice9* dev = nullptr;
    void SetUp() override   { dev = MakeHeadless(); ASSERT_NE(dev, nullptr); }
    void TearDown() override { if (dev) { dev->Release(); dev = nullptr; } }
};

// NRM: normalize(c0.xyz) with c0=(1,1,0,0) → (1/√2, 1/√2, 0).
// Golden locks coverage + correct normalize output at every triangle pixel.
TEST_F(D3D9DrawGoldenTests, OpcodeNRM_NormalizeVector)
{
    const float c0[4] = { 1.f, 1.f, 0.f, 0.f };
    auto pixels = DrawSolid(dev, D3D9SW_SHADER_DIR "/ps_nrm.o", c0);
    ASSERT_FALSE(pixels.empty()) << "ps_nrm.o not found — run compile.sh";

    auto result = CheckGolden("d3d9_opcode_nrm_64x64", pixels.data(), kW, kH, 0.f);
    EXPECT_TRUE(result.passed) << result.message;

    // Sanity: center pixel R ≈ 0.707 (normalized component), G ≈ 0.707, B = 0.
    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_NEAR(ctr[0], 1.f / std::sqrt(2.f), 1.f / 255.f) << "center R";
    EXPECT_NEAR(ctr[1], 1.f / std::sqrt(2.f), 1.f / 255.f) << "center G";
    EXPECT_NEAR(ctr[2], 0.f,                  1.f / 255.f) << "center B";
}

// POW: pow(0.5, 2.0) = 0.25. Golden locks correct power function output.
TEST_F(D3D9DrawGoldenTests, OpcodePOW_PowerFunction)
{
    const float c0[4] = { 0.5f, 2.f, 0.f, 0.f };
    auto pixels = DrawSolid(dev, D3D9SW_SHADER_DIR "/ps_pow.o", c0);
    ASSERT_FALSE(pixels.empty()) << "ps_pow.o not found — run compile.sh";

    auto result = CheckGolden("d3d9_opcode_pow_64x64", pixels.data(), kW, kH, 0.f);
    EXPECT_TRUE(result.passed) << result.message;

    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_NEAR(ctr[0], 0.25f, 1.f / 255.f) << "center R (pow(0.5,2)=0.25)";
}

// SINCOS: sincos(pi/6) → cos=√3/2≈0.866 (R), sin=0.5 (G).
TEST_F(D3D9DrawGoldenTests, OpcodeSINCOS_SineAndCosine)
{
    constexpr float kPi6 = 3.14159265f / 6.f;
    const float c0[4] = { kPi6, 0.f, 0.f, 0.f };
    auto pixels = DrawSolid(dev, D3D9SW_SHADER_DIR "/ps_sincos.o", c0);
    ASSERT_FALSE(pixels.empty()) << "ps_sincos.o not found — run compile.sh";

    auto result = CheckGolden("d3d9_opcode_sincos_64x64", pixels.data(), kW, kH, 0.f);
    EXPECT_TRUE(result.passed) << result.message;

    const float* ctr = pixels.data() + (kH/2 * kW + kW/2) * 4;
    EXPECT_NEAR(ctr[0], std::cos(kPi6), 1.f / 255.f) << "center R (cos(pi/6))";
    EXPECT_NEAR(ctr[1], std::sin(kPi6), 1.f / 255.f) << "center G (sin(pi/6))";
    EXPECT_NEAR(ctr[2], 0.f,            1.f / 255.f) << "center B";
}
