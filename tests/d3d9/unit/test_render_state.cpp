#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <cstdint>
#include <cstring>

namespace
{
    constexpr UINT kW = 32;
    constexpr UINT kH = 32;

    IDirect3DDevice9* MakeHeadless(UINT w = 8, UINT h = 8)
    {
        IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
        D3DPRESENT_PARAMETERS pp = {};
        pp.BackBufferWidth  = w; pp.BackBufferHeight = h;
        pp.BackBufferFormat = D3DFMT_A8R8G8B8;
        pp.BackBufferCount  = 1; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.Windowed = TRUE;
        IDirect3DDevice9* dev = nullptr;
        d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                          D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
        d3d->Release();
        return dev;
    }

    // Minimal CW-wound passthrough VS and constant-color PS (same as test_draw_triangle.cpp).
    constexpr DWORD kEnd = 0x80000000u;
    DWORD EncRT(DWORD t) { return ((t&7u)<<D3DSP_REGTYPE_SHIFT)|((t&0x18u)<<D3DSP_REGTYPE_SHIFT2); }
    DWORD Op(DWORD op, DWORD len) { return (op&D3DSI_OPCODE_MASK)|((len&0xFu)<<D3DSI_INSTLENGTH_SHIFT); }
    DWORD Dst(DWORD t, DWORD n, DWORD wm=0xFu) { return kEnd|EncRT(t)|(n&D3DSP_REGNUM_MASK)|((wm&0xFu)<<16); }
    DWORD Src(DWORD t, DWORD n) { return kEnd|EncRT(t)|(n&D3DSP_REGNUM_MASK)|D3DSP_NOSWIZZLE; }
    DWORD DU(DWORD u, DWORD i=0) { return kEnd|((u&0xFu)<<D3DSP_DCL_USAGE_SHIFT)|((i&0xFu)<<D3DSP_DCL_USAGEINDEX_SHIFT); }

    std::vector<DWORD> PosVS() {
        return { D3DVS_VERSION(3,0), Op(D3DSIO_DCL,2),DU(D3DDECLUSAGE_POSITION),Dst(D3DSPR_INPUT,0),
                 Op(D3DSIO_DCL,2),DU(D3DDECLUSAGE_POSITION),Dst(D3DSPR_OUTPUT,0),
                 Op(D3DSIO_MOV,2),Dst(D3DSPR_OUTPUT,0),Src(D3DSPR_INPUT,0), D3DVS_END() };
    }
    std::vector<DWORD> ConstPS() {
        return { D3DPS_VERSION(3,0), Op(D3DSIO_MOV,2),Dst(D3DSPR_COLOROUT,0),Src(D3DSPR_CONST,0), D3DPS_END() };
    }

    // Draw one CW triangle; return center pixel bytes [B,G,R,A].
    struct Px4 { uint8_t b, g, r, a; };
    Px4 DrawAndSampleCenter(IDirect3DDevice9* dev, float rgba[4])
    {
        dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0,0,0,0), 1.f, 0);

        struct V { float x,y,z; };
        const V verts[3] = { {-0.8f,-0.8f,0.5f},{0.0f,0.8f,0.5f},{0.8f,-0.8f,0.5f} };
        IDirect3DVertexBuffer9* vb = nullptr;
        dev->CreateVertexBuffer(sizeof(verts),0,0,D3DPOOL_DEFAULT,&vb,nullptr);
        void* m=nullptr; vb->Lock(0,sizeof(verts),&m,0); memcpy(m,verts,sizeof(verts)); vb->Unlock();
        dev->SetStreamSource(0,vb,0,sizeof(V));

        const D3DVERTEXELEMENT9 elem[]={ {0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},D3DDECL_END() };
        IDirect3DVertexDeclaration9* decl=nullptr; dev->CreateVertexDeclaration(elem,&decl);
        dev->SetVertexDeclaration(decl);

        auto vsbc=PosVS(); IDirect3DVertexShader9* vs=nullptr; dev->CreateVertexShader(vsbc.data(),&vs); dev->SetVertexShader(vs);
        auto psbc=ConstPS(); IDirect3DPixelShader9* ps=nullptr; dev->CreatePixelShader(psbc.data(),&ps); dev->SetPixelShader(ps);
        dev->SetPixelShaderConstantF(0,rgba,1);

        dev->BeginScene();
        dev->DrawPrimitive(D3DPT_TRIANGLELIST,0,1);
        dev->EndScene();

        IDirect3DSurface9* bb=nullptr; dev->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&bb);
        D3DLOCKED_RECT lr={}; bb->LockRect(&lr,nullptr,D3DLOCK_READONLY);
        const auto* row = static_cast<const uint8_t*>(lr.pBits) + (kH/2)*lr.Pitch + (kW/2)*4;
        Px4 result = { row[0], row[1], row[2], row[3] };
        bb->UnlockRect(); bb->Release(); ps->Release(); vs->Release(); decl->Release(); vb->Release();
        return result;
    }
}

TEST(D3D9RenderState, Defaults_MatchD3D9Spec)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    DWORD v = 0;
    EXPECT_EQ(dev->GetRenderState(D3DRS_ZENABLE,          &v), S_OK); EXPECT_EQ(v, (DWORD)D3DZB_TRUE);
    EXPECT_EQ(dev->GetRenderState(D3DRS_FILLMODE,         &v), S_OK); EXPECT_EQ(v, (DWORD)D3DFILL_SOLID);
    EXPECT_EQ(dev->GetRenderState(D3DRS_ZWRITEENABLE,     &v), S_OK); EXPECT_EQ(v, (DWORD)TRUE);
    EXPECT_EQ(dev->GetRenderState(D3DRS_SRCBLEND,         &v), S_OK); EXPECT_EQ(v, (DWORD)D3DBLEND_ONE);
    EXPECT_EQ(dev->GetRenderState(D3DRS_DESTBLEND,        &v), S_OK); EXPECT_EQ(v, (DWORD)D3DBLEND_ZERO);
    EXPECT_EQ(dev->GetRenderState(D3DRS_CULLMODE,         &v), S_OK); EXPECT_EQ(v, (DWORD)D3DCULL_CCW);
    EXPECT_EQ(dev->GetRenderState(D3DRS_ZFUNC,            &v), S_OK); EXPECT_EQ(v, (DWORD)D3DCMP_LESSEQUAL);
    EXPECT_EQ(dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &v), S_OK); EXPECT_EQ(v, (DWORD)FALSE);
    EXPECT_EQ(dev->GetRenderState(D3DRS_STENCILENABLE,    &v), S_OK); EXPECT_EQ(v, (DWORD)FALSE);
    EXPECT_EQ(dev->GetRenderState(D3DRS_COLORWRITEENABLE, &v), S_OK); EXPECT_EQ(v, 0xFu);
    EXPECT_EQ(dev->GetRenderState(D3DRS_BLENDOP,          &v), S_OK); EXPECT_EQ(v, (DWORD)D3DBLENDOP_ADD);
    EXPECT_EQ(dev->GetRenderState(D3DRS_SCISSORTESTENABLE,&v), S_OK); EXPECT_EQ(v, (DWORD)FALSE);
    dev->Release();
}

TEST(D3D9RenderState, SetGet_RoundTrip)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    EXPECT_EQ(dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE), S_OK);
    DWORD v = 0xDEAD;
    EXPECT_EQ(dev->GetRenderState(D3DRS_CULLMODE, &v), S_OK);
    EXPECT_EQ(v, (DWORD)D3DCULL_NONE);

    EXPECT_EQ(dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE), S_OK);
    EXPECT_EQ(dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &v), S_OK);
    EXPECT_NE(v, (DWORD)FALSE);

    dev->Release();
}

TEST(D3D9RenderState, GetNullPtr_ReturnsError)
{
    IDirect3DDevice9* dev = MakeHeadless();
    EXPECT_EQ(dev->GetRenderState(D3DRS_CULLMODE, nullptr), D3DERR_INVALIDCALL);
    dev->Release();
}

// Alpha test: pixels with alpha below a threshold are discarded.
TEST(D3D9RenderState, AlphaTest_DiscardsBelowRef)
{
    IDirect3DDevice9* dev = MakeHeadless(kW, kH);
    ASSERT_NE(dev, nullptr);

    // Enable alpha test: keep pixels where PS_alpha >= 128 (GREATEREQUAL).
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    dev->SetRenderState(D3DRS_ALPHAREF, 128);

    // PS outputs full-red with alpha = 0.0 (fails alpha test → discarded).
    float redTransparent[4] = { 1.f, 0.f, 0.f, 0.f };
    Px4 discarded = DrawAndSampleCenter(dev, redTransparent);
    EXPECT_EQ(discarded.r, 0) << "pixel should be discarded (alpha=0 < ref=128)";

    // PS outputs full-red with alpha = 1.0 (passes alpha test → written).
    float redOpaque[4] = { 1.f, 0.f, 0.f, 1.f };
    Px4 kept = DrawAndSampleCenter(dev, redOpaque);
    EXPECT_EQ(kept.r, 0xFF) << "pixel should be written (alpha=255 >= ref=128)";

    dev->Release();
}

// Stencil ops: STENCILPASS=REPLACE should write the ref value.
TEST(D3D9RenderState, StencilOp_ReplaceOnPass)
{
    IDirect3DDevice9* dev = MakeHeadless(kW, kH);
    ASSERT_NE(dev, nullptr);

    IDirect3DSurface9* dsv = nullptr;
    ASSERT_EQ(dev->CreateDepthStencilSurface(kW, kH, D3DFMT_D24S8,
              D3DMULTISAMPLE_NONE, 0, TRUE, &dsv, nullptr), S_OK);
    dev->SetDepthStencilSurface(dsv);
    dev->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0, 1.f, 0);

    // Disable depth test — this test is specifically about stencil op behaviour.
    // With depth test enabled and the D24S8 DSV, depth comparison can reject the
    // triangle before the stencil pass op runs (depth+stencil interaction is
    // covered by the depth/stencil golden tests).
    dev->SetRenderState(D3DRS_ZENABLE,        FALSE);
    dev->SetRenderState(D3DRS_STENCILENABLE,  TRUE);
    dev->SetRenderState(D3DRS_STENCILFUNC,    D3DCMP_ALWAYS);
    dev->SetRenderState(D3DRS_STENCILPASS,    D3DSTENCILOP_REPLACE);
    dev->SetRenderState(D3DRS_STENCILREF,     0x42);
    dev->SetRenderState(D3DRS_STENCILMASK,    0xFF);
    dev->SetRenderState(D3DRS_STENCILWRITEMASK, 0xFF);

    float red[4] = { 1.f, 0.f, 0.f, 1.f };
    Px4 bbPx = DrawAndSampleCenter(dev, red);
    EXPECT_EQ(bbPx.r, 0xFF) << "backbuffer should be red — triangle not rendering with DSV bound";

    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(dsv->LockRect(&lr, nullptr, D3DLOCK_READONLY), S_OK);
    const auto* row = static_cast<const uint8_t*>(lr.pBits) + (kH/2)*lr.Pitch;
    uint32_t pix = reinterpret_cast<const uint32_t*>(row)[kW/2];
    uint8_t stencil = static_cast<uint8_t>(pix >> 24);
    EXPECT_EQ(stencil, 0x42) << "stencil should be 0x42 (REPLACE with ref=0x42)";
    dsv->UnlockRect();

    dsv->Release();
    dev->Release();
}

// Separate alpha blend: srcBlendAlpha / destBlendAlpha applied independently.
TEST(D3D9RenderState, SeparateAlphaBlend_RoundTrip)
{
    IDirect3DDevice9* dev = MakeHeadless();
    // Verify the states round-trip through SetRenderState/GetRenderState.
    dev->SetRenderState(D3DRS_SRCBLENDALPHA,  D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_BLENDOPALPHA,   D3DBLENDOP_ADD);
    DWORD v = 0;
    EXPECT_EQ(dev->GetRenderState(D3DRS_SRCBLENDALPHA,  &v), S_OK); EXPECT_EQ(v, (DWORD)D3DBLEND_SRCALPHA);
    EXPECT_EQ(dev->GetRenderState(D3DRS_DESTBLENDALPHA, &v), S_OK); EXPECT_EQ(v, (DWORD)D3DBLEND_INVSRCALPHA);
    EXPECT_EQ(dev->GetRenderState(D3DRS_BLENDOPALPHA,   &v), S_OK); EXPECT_EQ(v, (DWORD)D3DBLENDOP_ADD);
    dev->Release();
}

TEST(D3D9RenderState, TextureStageState_DefaultsAndRoundTrip)
{
    IDirect3DDevice9* dev = MakeHeadless();
    DWORD v = 0;
    // Stage 0 defaults
    EXPECT_EQ(dev->GetTextureStageState(0, D3DTSS_COLOROP,  &v), S_OK); EXPECT_EQ(v, (DWORD)D3DTOP_MODULATE);
    EXPECT_EQ(dev->GetTextureStageState(0, D3DTSS_COLORARG1,&v), S_OK); EXPECT_EQ(v, (DWORD)D3DTA_TEXTURE);
    EXPECT_EQ(dev->GetTextureStageState(0, D3DTSS_ALPHAOP,  &v), S_OK); EXPECT_EQ(v, (DWORD)D3DTOP_SELECTARG1);
    // Stage 1 default colour-op = DISABLE
    EXPECT_EQ(dev->GetTextureStageState(1, D3DTSS_COLOROP,  &v), S_OK); EXPECT_EQ(v, (DWORD)D3DTOP_DISABLE);
    // Round-trip
    EXPECT_EQ(dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD), S_OK);
    EXPECT_EQ(dev->GetTextureStageState(0, D3DTSS_COLOROP, &v), S_OK); EXPECT_EQ(v, (DWORD)D3DTOP_ADD);
    dev->Release();
}

TEST(D3D9RenderState, Material_RoundTrip)
{
    IDirect3DDevice9* dev = MakeHeadless();
    D3DMATERIAL9 mat = {};
    mat.Diffuse  = { 1.f, 0.f, 0.f, 1.f };
    mat.Specular = { 0.f, 1.f, 0.f, 1.f };
    mat.Power    = 32.f;
    EXPECT_EQ(dev->SetMaterial(&mat), S_OK);
    D3DMATERIAL9 got = {};
    EXPECT_EQ(dev->GetMaterial(&got), S_OK);
    EXPECT_EQ(got.Diffuse.r, 1.f);
    EXPECT_EQ(got.Specular.g, 1.f);
    EXPECT_EQ(got.Power, 32.f);
    dev->Release();
}

TEST(D3D9RenderState, Light_RoundTrip)
{
    IDirect3DDevice9* dev = MakeHeadless();
    D3DLIGHT9 light = {};
    light.Type      = D3DLIGHT_DIRECTIONAL;
    light.Diffuse   = { 1.f, 1.f, 1.f, 1.f };
    light.Direction = { 0.f, -1.f, 0.f };
    EXPECT_EQ(dev->SetLight(0, &light), S_OK);
    EXPECT_EQ(dev->LightEnable(0, TRUE), S_OK);

    D3DLIGHT9 got = {};
    EXPECT_EQ(dev->GetLight(0, &got), S_OK);
    EXPECT_EQ(got.Type, D3DLIGHT_DIRECTIONAL);
    EXPECT_EQ(got.Direction.y, -1.f);

    BOOL enabled = FALSE;
    EXPECT_EQ(dev->GetLightEnable(0, &enabled), S_OK);
    EXPECT_NE(enabled, FALSE);
    dev->Release();
}

TEST(D3D9RenderState, ClipPlane_RoundTrip)
{
    IDirect3DDevice9* dev = MakeHeadless();
    const float plane[4] = { 0.f, 1.f, 0.f, -5.f };
    EXPECT_EQ(dev->SetClipPlane(0, plane), S_OK);
    float got[4] = {};
    EXPECT_EQ(dev->GetClipPlane(0, got), S_OK);
    EXPECT_EQ(got[1], 1.f);
    EXPECT_EQ(got[3], -5.f);
    dev->Release();
}

// StateBlock: CreateStateBlock captures RS and Apply restores it.
TEST(D3D9RenderState, CreateStateBlock_CaptureAndApply)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    // Set a distinctive render state
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW), S_OK);

    // Capture via CreateStateBlock
    IDirect3DStateBlock9* sb = nullptr;
    ASSERT_EQ(dev->CreateStateBlock(D3DSBT_ALL, &sb), S_OK);
    ASSERT_NE(sb, nullptr);

    // Change the state after capturing
    ASSERT_EQ(dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE), S_OK);
    DWORD v = 0;
    ASSERT_EQ(dev->GetRenderState(D3DRS_CULLMODE, &v), S_OK);
    EXPECT_EQ(v, static_cast<DWORD>(D3DCULL_NONE));

    // Apply the state block — should restore captured value
    ASSERT_EQ(sb->Apply(), S_OK);
    ASSERT_EQ(dev->GetRenderState(D3DRS_CULLMODE, &v), S_OK);
    EXPECT_EQ(v, static_cast<DWORD>(D3DCULL_CW));

    sb->Release();
    dev->Release();
}

// StateBlock: BeginStateBlock/EndStateBlock captures states set during recording.
TEST(D3D9RenderState, BeginEndStateBlock_CapturesRecordedState)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    ASSERT_EQ(dev->BeginStateBlock(), S_OK);
    ASSERT_EQ(dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME), S_OK);
    IDirect3DStateBlock9* sb = nullptr;
    ASSERT_EQ(dev->EndStateBlock(&sb), S_OK);
    ASSERT_NE(sb, nullptr);

    // Change state after EndStateBlock
    ASSERT_EQ(dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID), S_OK);

    // Apply must restore D3DFILL_WIREFRAME
    ASSERT_EQ(sb->Apply(), S_OK);
    DWORD v = 0;
    ASSERT_EQ(dev->GetRenderState(D3DRS_FILLMODE, &v), S_OK);
    EXPECT_EQ(v, static_cast<DWORD>(D3DFILL_WIREFRAME));

    sb->Release();
    dev->Release();
}

// StateBlock::Capture() re-snapshots after creation.
TEST(D3D9RenderState, StateBlock_Recapture)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    IDirect3DStateBlock9* sb = nullptr;
    ASSERT_EQ(dev->CreateStateBlock(D3DSBT_ALL, &sb), S_OK);

    // Change state and re-capture
    ASSERT_EQ(dev->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE), S_OK);
    ASSERT_EQ(sb->Capture(), S_OK);

    // Change again
    ASSERT_EQ(dev->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE), S_OK);

    // Apply → should restore FALSE (from re-capture)
    ASSERT_EQ(sb->Apply(), S_OK);
    DWORD v = D3DZB_TRUE;
    ASSERT_EQ(dev->GetRenderState(D3DRS_ZENABLE, &v), S_OK);
    EXPECT_EQ(v, static_cast<DWORD>(D3DZB_FALSE));

    sb->Release();
    dev->Release();
}

// D3D9 spec defaults for material-source render states.
TEST(D3D9RenderState, MaterialSource_Defaults)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    DWORD v = 0;
    // D3DRS_COLORVERTEX defaults to TRUE
    ASSERT_EQ(dev->GetRenderState(D3DRS_COLORVERTEX, &v), S_OK);
    EXPECT_EQ(v, static_cast<DWORD>(TRUE)) << "D3DRS_COLORVERTEX default must be TRUE";

    // D3DRS_DIFFUSEMATERIALSOURCE defaults to D3DMCS_COLOR1 (= 1)
    ASSERT_EQ(dev->GetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, &v), S_OK);
    EXPECT_EQ(v, static_cast<DWORD>(D3DMCS_COLOR1))
        << "D3DRS_DIFFUSEMATERIALSOURCE default must be D3DMCS_COLOR1";

    // D3DRS_SPECULARMATERIALSOURCE defaults to D3DMCS_COLOR2 (= 2)
    ASSERT_EQ(dev->GetRenderState(D3DRS_SPECULARMATERIALSOURCE, &v), S_OK);
    EXPECT_EQ(v, static_cast<DWORD>(D3DMCS_COLOR2))
        << "D3DRS_SPECULARMATERIALSOURCE default must be D3DMCS_COLOR2";

    // D3DRS_AMBIENTMATERIALSOURCE defaults to D3DMCS_MATERIAL (= 0)
    ASSERT_EQ(dev->GetRenderState(D3DRS_AMBIENTMATERIALSOURCE, &v), S_OK);
    EXPECT_EQ(v, static_cast<DWORD>(D3DMCS_MATERIAL))
        << "D3DRS_AMBIENTMATERIALSOURCE default must be D3DMCS_MATERIAL";

    // D3DRS_EMISSIVEMATERIALSOURCE defaults to D3DMCS_MATERIAL (= 0)
    ASSERT_EQ(dev->GetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, &v), S_OK);
    EXPECT_EQ(v, static_cast<DWORD>(D3DMCS_MATERIAL))
        << "D3DRS_EMISSIVEMATERIALSOURCE default must be D3DMCS_MATERIAL";

    dev->Release();
}
