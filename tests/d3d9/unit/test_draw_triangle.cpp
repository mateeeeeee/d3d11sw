#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <cstdint>
#include <cstring>
#include <vector>

namespace
{
    constexpr UINT kW = 32;
    constexpr UINT kH = 32;

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

    // ---- SM3 token builders (mirror of the helpers in test_shaders.cpp) ----
    constexpr DWORD kTokEnd = 0x80000000u;
    DWORD EncRegType(DWORD type)
    {
        DWORD lo = (type & 0x7u)  << D3DSP_REGTYPE_SHIFT;
        DWORD hi = (type & 0x18u) << D3DSP_REGTYPE_SHIFT2;
        return lo | hi;
    }
    DWORD Opcode(DWORD op, DWORD instLen)
    {
        return (op & D3DSI_OPCODE_MASK) | ((instLen & 0xFu) << D3DSI_INSTLENGTH_SHIFT);
    }
    DWORD Dst(DWORD type, DWORD num, DWORD writeMask = 0xFu)
    {
        return kTokEnd | EncRegType(type) | (num & D3DSP_REGNUM_MASK) | ((writeMask & 0xFu) << 16);
    }
    DWORD Src(DWORD type, DWORD num)
    {
        return kTokEnd | EncRegType(type) | (num & D3DSP_REGNUM_MASK) | D3DSP_NOSWIZZLE;
    }
    DWORD DclUsage(DWORD usage, DWORD usageIdx = 0)
    {
        return kTokEnd
             | ((usage    & 0xFu) << D3DSP_DCL_USAGE_SHIFT)
             | ((usageIdx & 0xFu) << D3DSP_DCL_USAGEINDEX_SHIFT);
    }

    // vs_3_0:  dcl_position v0 / dcl_position o0 / mov o0, v0
    std::vector<DWORD> PassthroughVS()
    {
        return {
            D3DVS_VERSION(3, 0),
            Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_POSITION), Dst(D3DSPR_INPUT,  0),
            Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_POSITION), Dst(D3DSPR_OUTPUT, 0),
            Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_OUTPUT, 0), Src(D3DSPR_INPUT, 0),
            D3DVS_END()
        };
    }

    // ps_3_0:  mov oC0, c0   — writes a constant-buffer-sourced color.
    std::vector<DWORD> ConstantColorPS()
    {
        return {
            D3DPS_VERSION(3, 0),
            Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_COLOROUT, 0), Src(D3DSPR_CONST, 0),
            D3DPS_END()
        };
    }
}

// MVP bring-up: SM3 VS + SM3 PS draw a clip-space triangle, constant red color
// sourced from c0. Verifies the full path Snapshot → Builder → SWRasterizer.
TEST(D3D9DrawTriangle, ConstantColorTriangle_RendersRedInCenter)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    // Black backbuffer to start so missed pixels stay distinguishable from red.
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF, 0, 0, 0), 1.f, 0), S_OK);

    // Triangle in clip space (Y-up), CW winding = front-facing under D3D9's
    // default D3DCULL_CCW (cull CCW = back-facing triangles).
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

    auto vsbc = PassthroughVS();
    IDirect3DVertexShader9* vs = nullptr;
    ASSERT_EQ(dev->CreateVertexShader(vsbc.data(), &vs), S_OK);
    ASSERT_EQ(dev->SetVertexShader(vs), S_OK);

    auto psbc = ConstantColorPS();
    IDirect3DPixelShader9* ps = nullptr;
    ASSERT_EQ(dev->CreatePixelShader(psbc.data(), &ps), S_OK);
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

    auto pixelAt = [&](UINT x, UINT y) {
        return static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch + x * 4;
    };

    // A8R8G8B8 → DXGI B8G8R8A8: bytes are {B, G, R, A}.
    // Center pixel is squarely inside the triangle.
    const uint8_t* ctr = pixelAt(kW / 2, kH / 2);
    EXPECT_EQ(ctr[0], 0x00) << "center B";
    EXPECT_EQ(ctr[1], 0x00) << "center G";
    EXPECT_EQ(ctr[2], 0xFF) << "center R";
    EXPECT_EQ(ctr[3], 0xFF) << "center A";

    // Top corners are above the apex (y > 0.8), so they must remain background.
    const uint8_t* tl = pixelAt(0, 0);
    EXPECT_EQ(tl[2], 0x00) << "top-left R (should be background)";
    const uint8_t* tr = pixelAt(kW - 1, 0);
    EXPECT_EQ(tr[2], 0x00) << "top-right R (should be background)";

    ASSERT_EQ(bb->UnlockRect(), S_OK);
    bb->Release();
    ps->Release();
    vs->Release();
    decl->Release();
    vb->Release();
    dev->Release();
}

TEST(D3D9DrawTriangle, DrawPrimitiveUP_RendersTriangle)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF,0,0,0), 1.f, 0), S_OK);

    struct V { float x, y, z; };
    const V verts[3] = { {-0.8f,-0.8f,0.5f}, {0.0f,0.8f,0.5f}, {0.8f,-0.8f,0.5f} };

    const D3DVERTEXELEMENT9 declElems[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        D3DDECL_END(),
    };
    IDirect3DVertexDeclaration9* decl = nullptr;
    ASSERT_EQ(dev->CreateVertexDeclaration(declElems, &decl), S_OK);
    ASSERT_EQ(dev->SetVertexDeclaration(decl), S_OK);

    auto vsbc = PassthroughVS();
    IDirect3DVertexShader9* vs = nullptr;
    ASSERT_EQ(dev->CreateVertexShader(vsbc.data(), &vs), S_OK);
    ASSERT_EQ(dev->SetVertexShader(vs), S_OK);

    auto psbc = ConstantColorPS();
    IDirect3DPixelShader9* ps = nullptr;
    ASSERT_EQ(dev->CreatePixelShader(psbc.data(), &ps), S_OK);
    ASSERT_EQ(dev->SetPixelShader(ps), S_OK);

    const float red[4] = { 1.f, 0.f, 0.f, 1.f };
    ASSERT_EQ(dev->SetPixelShaderConstantF(0, red, 1), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    // UP draw — no VB bound at all.
    ASSERT_EQ(dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, verts, sizeof(V)), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    IDirect3DSurface9* bb = nullptr;
    ASSERT_EQ(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb), S_OK);
    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(bb->LockRect(&lr, nullptr, D3DLOCK_READONLY), S_OK);
    const uint8_t* ctr = static_cast<const uint8_t*>(lr.pBits)
                       + (kH/2) * lr.Pitch + (kW/2) * 4;
    EXPECT_EQ(ctr[2], 0xFF) << "center R should be red";
    ASSERT_EQ(bb->UnlockRect(), S_OK);
    bb->Release();
    ps->Release();
    vs->Release();
    decl->Release();
    dev->Release();
}
