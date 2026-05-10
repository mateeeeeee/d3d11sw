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

    std::vector<float> ReadbackRGBA(IDirect3DDevice9* dev)
    {
        IDirect3DSurface9* bb = nullptr;
        dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
        D3DLOCKED_RECT lr = {};
        bb->LockRect(&lr, nullptr, D3DLOCK_READONLY);
        std::vector<float> pixels(kW * kH * 4);
        for (UINT y = 0; y < kH; ++y)
        {
            const uint8_t* row = static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch;
            for (UINT x = 0; x < kW; ++x)
            {
                UINT i = (y * kW + x) * 4;
                pixels[i + 0] = row[x * 4 + 2] / 255.f;  // R
                pixels[i + 1] = row[x * 4 + 1] / 255.f;  // G
                pixels[i + 2] = row[x * 4 + 0] / 255.f;  // B
                pixels[i + 3] = row[x * 4 + 3] / 255.f;  // A
            }
        }
        bb->UnlockRect();
        bb->Release();
        return pixels;
    }
}

struct D3D9InstancedDrawTests : ::testing::Test {};

// Three instances of the same small triangle rendered at different x-offsets,
// each with a distinct per-instance colour (red, green, blue). Verifies that
// SetStreamSourceFreq wires per-instance stream data correctly: the per-vertex
// geometry is reused across all instances, and the colour/offset advances once
// per instance (stepRate=1).
TEST_F(D3D9InstancedDrawTests, ThreeColoredTriangles)
{
    auto vsbc = ReadBytecode(D3D9SW_SHADER_DIR "/vs_instanced.o");
    auto psbc = ReadBytecode(D3D9SW_SHADER_DIR "/ps_color.o");
    ASSERT_FALSE(vsbc.empty()) << "vs_instanced.o not found — run tests/d3d9/shaders/compile.sh";
    ASSERT_FALSE(psbc.empty()) << "ps_color.o not found — run tests/d3d9/shaders/compile.sh";

    IDirect3DDevice9* dev = MakeHeadless();
    ASSERT_NE(dev, nullptr);

    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF, 0, 0, 0), 1.f, 0), S_OK);

    // Stream 0: per-vertex positions — small triangle centered at the origin.
    struct Vert { float x, y, z; };
    const Vert verts[3] = {
        {  0.00f,  0.25f, 0.5f },
        {  0.18f, -0.18f, 0.5f },
        { -0.18f, -0.18f, 0.5f },
    };
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(verts), 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);
    void* m = nullptr;
    ASSERT_EQ(vb->Lock(0, sizeof(verts), &m, 0), S_OK);
    std::memcpy(m, verts, sizeof(verts));
    ASSERT_EQ(vb->Unlock(), S_OK);

    // Stream 1: per-instance data — NDC x/y offset + RGBA colour.
    struct InstData { float ox, oy, r, g, b, a; };
    const InstData instances[3] = {
        { -0.55f, 0.f, 1.f, 0.f, 0.f, 1.f },   // left,   red
        {  0.00f, 0.f, 0.f, 1.f, 0.f, 1.f },   // centre, green
        {  0.55f, 0.f, 0.f, 0.f, 1.f, 1.f },   // right,  blue
    };
    IDirect3DVertexBuffer9* instVb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(sizeof(instances), 0, 0, D3DPOOL_DEFAULT, &instVb, nullptr), S_OK);
    ASSERT_EQ(instVb->Lock(0, sizeof(instances), &m, 0), S_OK);
    std::memcpy(m, instances, sizeof(instances));
    ASSERT_EQ(instVb->Unlock(), S_OK);

    // Index buffer: one triangle, indices 0-1-2.
    const uint16_t indices[3] = { 0, 1, 2 };
    IDirect3DIndexBuffer9* ib = nullptr;
    ASSERT_EQ(dev->CreateIndexBuffer(sizeof(indices), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib, nullptr), S_OK);
    ASSERT_EQ(ib->Lock(0, sizeof(indices), &m, 0), S_OK);
    std::memcpy(m, indices, sizeof(indices));
    ASSERT_EQ(ib->Unlock(), S_OK);

    // Vertex declaration: POSITION from stream 0 (per-vertex),
    // TEXCOORD0 (offset) + COLOR0 (colour) from stream 1 (per-instance).
    const D3DVERTEXELEMENT9 declElems[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 1,  0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 1,  8, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
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

    // Stream source freq: stream 0 = indexed vertex data with 3 instances,
    // stream 1 = per-instance data advancing once per instance.
    ASSERT_EQ(dev->SetStreamSource(0, vb,     0, sizeof(Vert)),     S_OK);
    ASSERT_EQ(dev->SetStreamSource(1, instVb, 0, sizeof(InstData)), S_OK);
    ASSERT_EQ(dev->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA  | 3), S_OK);
    ASSERT_EQ(dev->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1), S_OK);
    ASSERT_EQ(dev->SetIndices(ib), S_OK);

    ASSERT_EQ(dev->BeginScene(), S_OK);
    ASSERT_EQ(dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 3, 0, 1), S_OK);
    ASSERT_EQ(dev->EndScene(), S_OK);

    // Reset stream frequencies so subsequent state is clean.
    dev->SetStreamSourceFreq(0, 1);
    dev->SetStreamSourceFreq(1, 1);

    auto pixels = ReadbackRGBA(dev);
    auto result = CheckGolden("d3d9_instanced_triangles_64x64", pixels.data(), kW, kH, 0.f);
    EXPECT_TRUE(result.passed) << result.message;

    ib->Release();
    instVb->Release();
    vb->Release();
    ps->Release();
    vs->Release();
    decl->Release();
    dev->Release();
}
