#include <gtest/gtest.h>
#include <d3d9.h>
#include <cstdint>
#include <cstring>

namespace
{
    constexpr UINT kW = 32;
    constexpr UINT kH = 16;

    IDirect3DDevice9* CreateHeadlessDevice(D3DFORMAT fmt = D3DFMT_A8R8G8B8)
    {
        IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
        EXPECT_NE(d3d, nullptr);

        D3DPRESENT_PARAMETERS pp = {};
        pp.BackBufferWidth       = kW;
        pp.BackBufferHeight      = kH;
        pp.BackBufferFormat      = fmt;
        pp.BackBufferCount       = 1;
        pp.SwapEffect            = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow         = nullptr;
        pp.Windowed              = TRUE;
        pp.EnableAutoDepthStencil = FALSE;

        IDirect3DDevice9* dev = nullptr;
        HRESULT hr = d3d->CreateDevice(
            D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
        EXPECT_EQ(hr, S_OK);
        EXPECT_NE(dev, nullptr);

        d3d->Release();  // device holds its own ref
        return dev;
    }
}

TEST(D3D9Clear, CreateDevice_SucceedsWithHeadlessParams)
{
    IDirect3DDevice9* dev = CreateHeadlessDevice();
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(dev->TestCooperativeLevel(), S_OK);
    EXPECT_EQ(dev->GetNumberOfSwapChains(), 1u);
    ULONG refs = dev->Release();
    EXPECT_EQ(refs, 0u);
}

TEST(D3D9Clear, BackBuffer_HasExpectedDesc)
{
    IDirect3DDevice9* dev = CreateHeadlessDevice(D3DFMT_A8R8G8B8);
    IDirect3DSurface9* bb = nullptr;
    ASSERT_EQ(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb), S_OK);
    ASSERT_NE(bb, nullptr);

    D3DSURFACE_DESC desc = {};
    ASSERT_EQ(bb->GetDesc(&desc), S_OK);
    EXPECT_EQ(desc.Width,  kW);
    EXPECT_EQ(desc.Height, kH);
    EXPECT_EQ(desc.Format, D3DFMT_A8R8G8B8);
    EXPECT_EQ(desc.Type,   D3DRTYPE_SURFACE);

    bb->Release();
    dev->Release();
}

TEST(D3D9Clear, ClearToRed_FillsBackBuffer_A8R8G8B8)
{
    IDirect3DDevice9* dev = CreateHeadlessDevice(D3DFMT_A8R8G8B8);

    const D3DCOLOR red = D3DCOLOR_ARGB(0xFF, 0xFF, 0x00, 0x00);
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_TARGET, red, 1.f, 0), S_OK);

    IDirect3DSurface9* bb = nullptr;
    ASSERT_EQ(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb), S_OK);
    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(bb->LockRect(&lr, nullptr, D3DLOCK_READONLY), S_OK);

    // A8R8G8B8 → DXGI_FORMAT_B8G8R8A8_UNORM: bytes-in-memory are {B,G,R,A}.
    for (UINT y = 0; y < kH; ++y)
    {
        const uint8_t* row = static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch;
        for (UINT x = 0; x < kW; ++x)
        {
            EXPECT_EQ(row[x * 4 + 0], 0x00) << "pixel (" << x << "," << y << ") B";
            EXPECT_EQ(row[x * 4 + 1], 0x00) << "pixel (" << x << "," << y << ") G";
            EXPECT_EQ(row[x * 4 + 2], 0xFF) << "pixel (" << x << "," << y << ") R";
            EXPECT_EQ(row[x * 4 + 3], 0xFF) << "pixel (" << x << "," << y << ") A";
        }
    }

    ASSERT_EQ(bb->UnlockRect(), S_OK);
    bb->Release();
    dev->Release();
}

TEST(D3D9Clear, Present_NoWindow_NoOp)
{
    IDirect3DDevice9* dev = CreateHeadlessDevice();
    EXPECT_EQ(dev->Present(nullptr, nullptr, nullptr, nullptr), S_OK);
    dev->Release();
}

TEST(D3D9Clear, ClearDepth_D24S8_WritesExpectedValues)
{
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth  = kW; pp.BackBufferHeight = kH;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferCount  = 1;
    pp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    pp.Windowed         = TRUE;
    pp.EnableAutoDepthStencil = FALSE;
    IDirect3DDevice9* dev = nullptr;
    ASSERT_EQ(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev), S_OK);
    d3d->Release();

    IDirect3DSurface9* dsv = nullptr;
    ASSERT_EQ(dev->CreateDepthStencilSurface(kW, kH, D3DFMT_D24S8,
              D3DMULTISAMPLE_NONE, 0, TRUE, &dsv, nullptr), S_OK);
    ASSERT_EQ(dev->SetDepthStencilSurface(dsv), S_OK);

    // Clear depth to 0.5 and stencil to 0x42.
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
                         0, 0.5f, 0x42), S_OK);

    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(dsv->LockRect(&lr, nullptr, D3DLOCK_READONLY), S_OK);
    const uint32_t* row0 = static_cast<const uint32_t*>(lr.pBits);
    uint32_t pix = row0[0];
    // D24S8 layout: bits 31..8 = stencil (high byte) | depth24, bits 7..0 = free.
    // ReadDepthValue reads (pix & 0x00FFFFFF) / 16777215.f, WriteDepthValue keeps S in bits 31..24.
    uint8_t  stencil = static_cast<uint8_t>(pix >> 24);
    uint32_t depth24 = pix & 0x00FFFFFFu;
    float    depth   = depth24 / 16777215.f;
    EXPECT_EQ(stencil, 0x42);
    EXPECT_NEAR(depth, 0.5f, 1.f / 16777215.f * 2);
    ASSERT_EQ(dsv->UnlockRect(), S_OK);

    dsv->Release();
    dev->Release();
}

TEST(D3D9Clear, ClearDepthOnly_LeaveStencilUntouched)
{
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth  = kW; pp.BackBufferHeight = kH;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferCount  = 1; pp.SwapEffect = D3DSWAPEFFECT_DISCARD; pp.Windowed = TRUE;
    IDirect3DDevice9* dev = nullptr;
    ASSERT_EQ(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev), S_OK);
    d3d->Release();

    IDirect3DSurface9* dsv = nullptr;
    ASSERT_EQ(dev->CreateDepthStencilSurface(kW, kH, D3DFMT_D24S8,
              D3DMULTISAMPLE_NONE, 0, TRUE, &dsv, nullptr), S_OK);
    ASSERT_EQ(dev->SetDepthStencilSurface(dsv), S_OK);

    // First clear stencil to 0xFF.
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0, 1.f, 0xFF), S_OK);
    // Now clear only depth to 0.25 — stencil must stay 0xFF.
    ASSERT_EQ(dev->Clear(0, nullptr, D3DCLEAR_ZBUFFER, 0, 0.25f, 0), S_OK);

    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(dsv->LockRect(&lr, nullptr, D3DLOCK_READONLY), S_OK);
    const uint32_t* row0 = static_cast<const uint32_t*>(lr.pBits);
    uint8_t  stencil = static_cast<uint8_t>(row0[0] >> 24);
    uint32_t depth24 = row0[0] & 0x00FFFFFFu;
    EXPECT_EQ(stencil, 0xFF);
    EXPECT_NEAR(depth24 / 16777215.f, 0.25f, 1.f / 16777215.f * 2);
    ASSERT_EQ(dsv->UnlockRect(), S_OK);

    dsv->Release();
    dev->Release();
}
