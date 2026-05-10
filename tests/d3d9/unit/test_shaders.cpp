#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <cstdint>
#include <cstring>
#include <vector>

namespace
{
    constexpr UINT kW = 32;
    constexpr UINT kH = 16;

    IDirect3DDevice9* MakeHeadless()
    {
        IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
        D3DPRESENT_PARAMETERS pp = {};
        pp.BackBufferWidth       = kW;
        pp.BackBufferHeight      = kH;
        pp.BackBufferFormat      = D3DFMT_A8R8G8B8;
        pp.BackBufferCount       = 1;
        pp.SwapEffect            = D3DSWAPEFFECT_DISCARD;
        pp.Windowed              = TRUE;
        pp.EnableAutoDepthStencil = FALSE;
        IDirect3DDevice9* dev = nullptr;
        d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                          D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
        d3d->Release();
        return dev;
    }

    // ---- SM3 token builders (mirror of test_sm3_translator.cpp helpers) ---
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

    // Minimal vs_3_0 program: dcl_position v0 / dcl_position o0 / mov o0, v0
    std::vector<DWORD> MakeMinimalVS()
    {
        return {
            D3DVS_VERSION(3, 0),
            Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_POSITION), Dst(D3DSPR_INPUT,  0),
            Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_POSITION), Dst(D3DSPR_OUTPUT, 0),
            Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_OUTPUT, 0), Src(D3DSPR_INPUT, 0),
            D3DVS_END()
        };
    }

    // Minimal ps_3_0 program: dcl_color0 v0 / mov oC0, v0
    std::vector<DWORD> MakeMinimalPS()
    {
        return {
            D3DPS_VERSION(3, 0),
            Opcode(D3DSIO_DCL, 2), DclUsage(D3DDECLUSAGE_COLOR), Dst(D3DSPR_INPUT, 0),
            Opcode(D3DSIO_MOV, 2), Dst(D3DSPR_COLOROUT, 0), Src(D3DSPR_INPUT, 0),
            D3DPS_END()
        };
    }
}

TEST(D3D9Shaders, CreateVertexShader_ValidSM3_ReturnsSOK)
{
    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    auto bc = MakeMinimalVS();
    IDirect3DVertexShader9* vs = nullptr;
    HRESULT hr = dev->CreateVertexShader(bc.data(), &vs);
    EXPECT_EQ(hr, S_OK);
    ASSERT_NE(vs, nullptr);

    vs->Release();
    dev->Release();
}

TEST(D3D9Shaders, CreateVertexShader_NullBytecode_Rejected)
{
    IDirect3DDevice9* dev = MakeHeadless();
    IDirect3DVertexShader9* vs = nullptr;
    HRESULT hr = dev->CreateVertexShader(nullptr, &vs);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    EXPECT_EQ(vs, nullptr);
    dev->Release();
}

TEST(D3D9Shaders, CreateVertexShader_PSTokenStream_Rejected)
{
    // Wrong shader type in the token stream header (ps_3_0 handed to
    // CreateVertexShader) must not produce a VS object.
    IDirect3DDevice9* dev = MakeHeadless();
    auto bc = MakeMinimalPS();
    IDirect3DVertexShader9* vs = nullptr;
    HRESULT hr = dev->CreateVertexShader(bc.data(), &vs);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    EXPECT_EQ(vs, nullptr);
    dev->Release();
}

TEST(D3D9Shaders, SetGetVertexShader_RoundTrip)
{
    IDirect3DDevice9* dev = MakeHeadless();
    auto bc = MakeMinimalVS();
    IDirect3DVertexShader9* vs = nullptr;
    ASSERT_EQ(dev->CreateVertexShader(bc.data(), &vs), S_OK);

    ASSERT_EQ(dev->SetVertexShader(vs), S_OK);
    IDirect3DVertexShader9* got = nullptr;
    ASSERT_EQ(dev->GetVertexShader(&got), S_OK);
    EXPECT_EQ(got, vs);
    got->Release();

    // Setting null clears the slot.
    ASSERT_EQ(dev->SetVertexShader(nullptr), S_OK);
    got = reinterpret_cast<IDirect3DVertexShader9*>(0xdead);
    ASSERT_EQ(dev->GetVertexShader(&got), S_OK);
    EXPECT_EQ(got, nullptr);

    vs->Release();
    dev->Release();
}

TEST(D3D9Shaders, GetFunction_SizeQuery)
{
    IDirect3DDevice9* dev = MakeHeadless();
    auto bc = MakeMinimalVS();
    IDirect3DVertexShader9* vs = nullptr;
    ASSERT_EQ(dev->CreateVertexShader(bc.data(), &vs), S_OK);

    UINT sz = 0;
    EXPECT_EQ(vs->GetFunction(nullptr, &sz), S_OK);
    EXPECT_EQ(sz, bc.size() * sizeof(DWORD));

    std::vector<DWORD> buf(bc.size());
    UINT cap = static_cast<UINT>(buf.size() * sizeof(DWORD));
    EXPECT_EQ(vs->GetFunction(buf.data(), &cap), S_OK);
    EXPECT_EQ(std::memcmp(buf.data(), bc.data(), sz), 0);

    vs->Release();
    dev->Release();
}

TEST(D3D9Shaders, CreatePixelShader_ValidSM3_ReturnsSOK)
{
    IDirect3DDevice9* dev = MakeHeadless();
    auto bc = MakeMinimalPS();
    IDirect3DPixelShader9* ps = nullptr;
    EXPECT_EQ(dev->CreatePixelShader(bc.data(), &ps), S_OK);
    ASSERT_NE(ps, nullptr);
    ps->Release();
    dev->Release();
}

TEST(D3D9Shaders, CreatePixelShader_VSTokenStream_Rejected)
{
    IDirect3DDevice9* dev = MakeHeadless();
    auto bc = MakeMinimalVS();
    IDirect3DPixelShader9* ps = nullptr;
    EXPECT_EQ(dev->CreatePixelShader(bc.data(), &ps), D3DERR_INVALIDCALL);
    EXPECT_EQ(ps, nullptr);
    dev->Release();
}

TEST(D3D9Shaders, SetGetPixelShader_RoundTrip)
{
    IDirect3DDevice9* dev = MakeHeadless();
    auto bc = MakeMinimalPS();
    IDirect3DPixelShader9* ps = nullptr;
    ASSERT_EQ(dev->CreatePixelShader(bc.data(), &ps), S_OK);

    ASSERT_EQ(dev->SetPixelShader(ps), S_OK);
    IDirect3DPixelShader9* got = nullptr;
    ASSERT_EQ(dev->GetPixelShader(&got), S_OK);
    EXPECT_EQ(got, ps);
    got->Release();

    ps->Release();
    dev->Release();
}

TEST(D3D9Shaders, VSConstantF_RoundTrip)
{
    IDirect3DDevice9* dev = MakeHeadless();
    float in[8]  = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
    float out[8] = {};
    EXPECT_EQ(dev->SetVertexShaderConstantF(0, in, 2), S_OK);
    EXPECT_EQ(dev->GetVertexShaderConstantF(0, out, 2), S_OK);
    EXPECT_EQ(std::memcmp(in, out, sizeof(in)), 0);
    dev->Release();
}

TEST(D3D9Shaders, VSConstantF_UnsetReadsZero)
{
    IDirect3DDevice9* dev = MakeHeadless();
    float out[4] = {9, 9, 9, 9};
    EXPECT_EQ(dev->GetVertexShaderConstantF(100, out, 1), S_OK);
    EXPECT_EQ(out[0], 0.f);
    EXPECT_EQ(out[1], 0.f);
    EXPECT_EQ(out[2], 0.f);
    EXPECT_EQ(out[3], 0.f);
    dev->Release();
}

TEST(D3D9Shaders, VSConstantF_OutOfRange_Rejected)
{
    IDirect3DDevice9* dev = MakeHeadless();
    float in[4] = {0, 0, 0, 0};
    EXPECT_EQ(dev->SetVertexShaderConstantF(256, in, 1), D3DERR_INVALIDCALL);
    EXPECT_EQ(dev->SetVertexShaderConstantF(255, in, 2), D3DERR_INVALIDCALL);
    dev->Release();
}

TEST(D3D9Shaders, VSConstantI_RoundTrip)
{
    IDirect3DDevice9* dev = MakeHeadless();
    int in[4]  = {10, 20, 30, 40};
    int out[4] = {};
    EXPECT_EQ(dev->SetVertexShaderConstantI(3, in, 1), S_OK);
    EXPECT_EQ(dev->GetVertexShaderConstantI(3, out, 1), S_OK);
    EXPECT_EQ(out[0], 10);
    EXPECT_EQ(out[3], 40);
    dev->Release();
}

TEST(D3D9Shaders, VSConstantB_RoundTrip)
{
    IDirect3DDevice9* dev = MakeHeadless();
    BOOL in[2]  = {TRUE, FALSE};
    BOOL out[2] = {};
    EXPECT_EQ(dev->SetVertexShaderConstantB(5, in, 2), S_OK);
    EXPECT_EQ(dev->GetVertexShaderConstantB(5, out, 2), S_OK);
    EXPECT_NE(out[0], 0);
    EXPECT_EQ(out[1], 0);
    dev->Release();
}

TEST(D3D9Shaders, PSConstantF_RoundTrip_Separate_From_VS)
{
    IDirect3DDevice9* dev = MakeHeadless();
    float vsIn[4] = {1, 1, 1, 1};
    float psIn[4] = {2, 2, 2, 2};
    EXPECT_EQ(dev->SetVertexShaderConstantF(0, vsIn, 1), S_OK);
    EXPECT_EQ(dev->SetPixelShaderConstantF(0, psIn, 1), S_OK);

    float psOut[4] = {};
    EXPECT_EQ(dev->GetPixelShaderConstantF(0, psOut, 1), S_OK);
    EXPECT_EQ(psOut[0], 2.f);

    float vsOut[4] = {};
    EXPECT_EQ(dev->GetVertexShaderConstantF(0, vsOut, 1), S_OK);
    EXPECT_EQ(vsOut[0], 1.f);
    dev->Release();
}

TEST(D3D9Shaders, PSConstantF_OutOfRange_Rejected)
{
    IDirect3DDevice9* dev = MakeHeadless();
    float in[4] = {0, 0, 0, 0};
    EXPECT_EQ(dev->SetPixelShaderConstantF(224, in, 1), D3DERR_INVALIDCALL);
    dev->Release();
}
