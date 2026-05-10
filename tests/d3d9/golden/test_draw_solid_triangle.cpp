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
        pp.BackBufferWidth        = kW;
        pp.BackBufferHeight       = kH;
        pp.BackBufferFormat       = D3DFMT_A8R8G8B8;
        pp.BackBufferCount        = 1;
        pp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
        pp.Windowed               = TRUE;
        pp.EnableAutoDepthStencil = FALSE;
        IDirect3DDevice9* dev = nullptr;
        d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                          D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
        d3d->Release();
        return dev;
    }
}

struct D3D9DrawGoldenTests : ::testing::Test {};

// Sets a baseline reference for the rasterizer's pixel coverage / edge rules:
// any future drift in triangle coverage, viewport mapping, blending defaults,
// or PS constant-buffer plumbing will register here as a pixel-level diff.
TEST_F(D3D9DrawGoldenTests, SolidTriangle64x64)
{
    auto vsbc = ReadBytecode(D3D9SW_SHADER_DIR "/vs_passthrough.o");
    auto psbc = ReadBytecode(D3D9SW_SHADER_DIR "/ps_const_color.o");
    ASSERT_FALSE(vsbc.empty()) << "VS shader not found — run tests/d3d9/shaders/compile.sh";
    ASSERT_FALSE(psbc.empty()) << "PS shader not found — run tests/d3d9/shaders/compile.sh";

    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF, 0, 0, 0), 1.f, 0), S_OK);

    struct V { float x, y, z; };
    const V verts[3] = {
        {  0.0f,  0.5f, 0.5f },
        {  0.5f, -0.5f, 0.5f },
        { -0.5f, -0.5f, 0.5f },
    };
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);
    void* mapped = nullptr;
    ASSERT_EQ(vb->Lock(0, sizeof(verts), &mapped, 0), S_OK);
    std::memcpy(mapped, verts, sizeof(verts));
    ASSERT_EQ(vb->Unlock(), S_OK);
    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(V)), S_OK);

    const D3DVERTEXELEMENT9 declElems[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        D3DDECL_END(),
    };
    IDirect3DVertexDeclaration9* decl = nullptr;
    ASSERT_EQ(dev->CreateVertexDeclaration(declElems, &decl), S_OK);
    ASSERT_EQ(dev->SetVertexDeclaration(decl), S_OK);

    IDirect3DVertexShader9* vs = nullptr;
    ASSERT_EQ(dev->CreateVertexShader(reinterpret_cast<const DWORD*>(vsbc.data()), &vs), S_OK);
    ASSERT_EQ(dev->SetVertexShader(vs), S_OK);

    IDirect3DPixelShader9* ps = nullptr;
    ASSERT_EQ(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(psbc.data()), &ps), S_OK);
    ASSERT_EQ(dev->SetPixelShader(ps), S_OK);

    const float red[4] = { 1.f, 0.f, 0.f, 1.f };
    ASSERT_EQ(dev->SetPixelShaderConstantF(0, red, 1), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    ASSERT_EQ(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb), S_OK);
    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(bb->LockRect(&lr, nullptr, D3DLOCK_READONLY), S_OK);

    // Backbuffer is A8R8G8B8 → DXGI_FORMAT_B8G8R8A8_UNORM (bytes B,G,R,A).
    // The golden harness wants linear-float RGBA.
    std::vector<float> pixels(kW * kH * 4);
    for (UINT y = 0; y < kH; ++y)
    {
        const uint8_t* row = static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch;
        for (UINT x = 0; x < kW; ++x)
        {
            UINT idx = (y * kW + x) * 4;
            pixels[idx + 0] = row[x * 4 + 2] / 255.f;  // R
            pixels[idx + 1] = row[x * 4 + 1] / 255.f;  // G
            pixels[idx + 2] = row[x * 4 + 0] / 255.f;  // B
            pixels[idx + 3] = row[x * 4 + 3] / 255.f;  // A
        }
    }
    ASSERT_EQ(bb->UnlockRect(), S_OK);

    auto result = CheckGolden("d3d9_solid_triangle_64x64", pixels.data(), kW, kH, 0.f);
    EXPECT_TRUE(result.passed) << result.message;

    bb->Release();
    ps->Release();
    vs->Release();
    decl->Release();
    vb->Release();
    dev->Release();
}
