#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

// End-to-end shader-pipeline tests using bytecode compiled by real fxc.
//
// The hand-built-token tests (test_draw_triangle.cpp) cover the IR shape we
// expect; these tests cover the shapes fxc actually emits — implicit float4
// constructors lowered to mov + DEF, sig metadata, etc. — so a regression in
// the SM3 decoder surfaces here even when the IR-only tests still pass.

namespace
{
    constexpr UINT kW = 32;
    constexpr UINT kH = 32;

    std::vector<uint8_t> ReadFile(const char* path)
    {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) { return {}; }
        auto sz = f.tellg();
        f.seekg(0);
        std::vector<uint8_t> bytes(static_cast<size_t>(sz));
        f.read(reinterpret_cast<char*>(bytes.data()), sz);
        return bytes;
    }

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

TEST(D3D9FxcShaders, VsPassthrough_PsConstColor_DrawsRedTriangle)
{
    auto vsbc = ReadFile(D3D9SW_SHADER_DIR "/vs_passthrough.o");
    auto psbc = ReadFile(D3D9SW_SHADER_DIR "/ps_const_color.o");
    if (vsbc.empty() || psbc.empty())
    {
        GTEST_SKIP() << "FXC-compiled .o files missing — run "
                        "tests/d3d9/shaders/compile.sh first.";
    }

    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF, 0, 0, 0), 1.f, 0), S_OK);

    struct V { float x, y, z; };
    const V verts[3] = {
        { -0.8f, -0.8f, 0.5f },
        {  0.0f,  0.8f, 0.5f },
        {  0.8f, -0.8f, 0.5f },
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
    const uint8_t* ctr = static_cast<const uint8_t*>(lr.pBits)
                       + (kH / 2) * lr.Pitch + (kW / 2) * 4;
    EXPECT_EQ(ctr[0], 0x00) << "center B";
    EXPECT_EQ(ctr[1], 0x00) << "center G";
    EXPECT_EQ(ctr[2], 0xFF) << "center R";
    EXPECT_EQ(ctr[3], 0xFF) << "center A";
    ASSERT_EQ(bb->UnlockRect(), S_OK);

    bb->Release();
    ps->Release();
    vs->Release();
    decl->Release();
    vb->Release();
    dev->Release();
}

TEST(D3D9FxcShaders, VsColor_PsColor_InterpolatesVertexColor)
{
    auto vsbc = ReadFile(D3D9SW_SHADER_DIR "/vs_color.o");
    auto psbc = ReadFile(D3D9SW_SHADER_DIR "/ps_color.o");
    if (vsbc.empty() || psbc.empty())
    {
        GTEST_SKIP() << "FXC-compiled .o files missing — run "
                        "tests/d3d9/shaders/compile.sh first.";
    }

    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF, 0, 0, 0), 1.f, 0), S_OK);

    // Each vertex carries a primary color so the center pixel ends up as a
    // ~1/3-1/3-1/3 mix — i.e. all three channels should be non-zero.
    struct V { float x, y, z; float r, g, b, a; };
    const V verts[3] = {
        { -0.8f, -0.8f, 0.5f,  1.f, 0.f, 0.f, 1.f },
        {  0.0f,  0.8f, 0.5f,  0.f, 0.f, 1.f, 1.f },
        {  0.8f, -0.8f, 0.5f,  0.f, 1.f, 0.f, 1.f },
    };
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);
    void* mapped = nullptr;
    ASSERT_EQ(vb->Lock(0, sizeof(verts), &mapped, 0), S_OK);
    std::memcpy(mapped, verts, sizeof(verts));
    ASSERT_EQ(vb->Unlock(), S_OK);
    ASSERT_EQ(dev->SetStreamSource(0, vb, 0, sizeof(V)), S_OK);

    const D3DVERTEXELEMENT9 declElems[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
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

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    ASSERT_EQ(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb), S_OK);
    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(bb->LockRect(&lr, nullptr, D3DLOCK_READONLY), S_OK);
    const uint8_t* ctr = static_cast<const uint8_t*>(lr.pBits)
                       + (kH / 2) * lr.Pitch + (kW / 2) * 4;
    // BGRA: each channel should land somewhere in the interpolation range.
    EXPECT_GT(ctr[0], 0x10) << "center B (blue must be interpolated)";
    EXPECT_GT(ctr[1], 0x10) << "center G (green must be interpolated)";
    EXPECT_GT(ctr[2], 0x10) << "center R (red must be interpolated)";
    EXPECT_EQ(ctr[3], 0xFF) << "center A";
    ASSERT_EQ(bb->UnlockRect(), S_OK);

    bb->Release();
    ps->Release();
    vs->Release();
    decl->Release();
    vb->Release();
    dev->Release();
}
