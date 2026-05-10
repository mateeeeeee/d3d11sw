#include <gtest/gtest.h>
#include <d3d9.h>

TEST(D3D9Create, Direct3DCreate9ReturnsNonNull)
{
    IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ASSERT_NE(d3d9, nullptr);

    EXPECT_EQ(d3d9->GetAdapterCount(), 1u);

    ULONG refs = d3d9->Release();
    EXPECT_EQ(refs, 0u);
}

TEST(D3D9Create, QueryInterfaceIUnknown)
{
    IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ASSERT_NE(d3d9, nullptr);

    IUnknown* unk = nullptr;
    HRESULT hr = d3d9->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&unk));
    EXPECT_EQ(hr, S_OK);
    ASSERT_NE(unk, nullptr);
    unk->Release();

    d3d9->Release();
}

TEST(D3D9Create, Direct3DCreate9Ex_ReturnsExInterface)
{
    IDirect3D9Ex* d3d9ex = nullptr;
    HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    EXPECT_EQ(hr, S_OK);
    EXPECT_NE(d3d9ex, nullptr);

    // Must also be queryable as IDirect3D9
    IDirect3D9* d3d9 = nullptr;
    EXPECT_EQ(d3d9ex->QueryInterface(IID_IDirect3D9, reinterpret_cast<void**>(&d3d9)), S_OK);
    EXPECT_NE(d3d9, nullptr);
    if (d3d9) { d3d9->Release(); }

    d3d9ex->Release();
}

TEST(D3D9Create, GetAdapterIdentifier_ReturnsDescriptiveStrings)
{
    IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ASSERT_NE(d3d9, nullptr);

    D3DADAPTER_IDENTIFIER9 id = {};
    ASSERT_EQ(d3d9->GetAdapterIdentifier(0, 0, &id), S_OK);
    EXPECT_GT(std::strlen(id.Driver),      0u) << "Driver string should be non-empty";
    EXPECT_GT(std::strlen(id.Description), 0u) << "Description should be non-empty";

    // Out-of-range adapter must fail.
    EXPECT_NE(d3d9->GetAdapterIdentifier(1, 0, &id), S_OK);

    d3d9->Release();
}

TEST(D3D9Create, GetAdapterModeCount_SupportedFormats)
{
    IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ASSERT_NE(d3d9, nullptr);

    // Primary 32-bit format must have at least one mode.
    UINT count32 = d3d9->GetAdapterModeCount(0, D3DFMT_X8R8G8B8);
    EXPECT_GT(count32, 0u) << "X8R8G8B8 must expose at least one mode";

    // Unsupported format returns 0.
    UINT countUnk = d3d9->GetAdapterModeCount(0, D3DFMT_UNKNOWN);
    EXPECT_EQ(countUnk, 0u);

    // Out-of-range adapter returns 0.
    UINT countBad = d3d9->GetAdapterModeCount(1, D3DFMT_X8R8G8B8);
    EXPECT_EQ(countBad, 0u);

    d3d9->Release();
}

TEST(D3D9Create, EnumAdapterModes_ValidRange)
{
    IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ASSERT_NE(d3d9, nullptr);

    UINT count = d3d9->GetAdapterModeCount(0, D3DFMT_X8R8G8B8);
    ASSERT_GT(count, 0u);

    // First mode: must have positive dimensions and correct format.
    D3DDISPLAYMODE mode = {};
    ASSERT_EQ(d3d9->EnumAdapterModes(0, D3DFMT_X8R8G8B8, 0, &mode), S_OK);
    EXPECT_GT(mode.Width,  0u);
    EXPECT_GT(mode.Height, 0u);
    EXPECT_EQ(mode.Format, D3DFMT_X8R8G8B8);

    // Current display mode (1920×1080) must appear in the list.
    D3DDISPLAYMODE current = {};
    ASSERT_EQ(d3d9->GetAdapterDisplayMode(0, &current), S_OK);
    bool found = false;
    for (UINT i = 0; i < count; ++i)
    {
        D3DDISPLAYMODE m = {};
        d3d9->EnumAdapterModes(0, D3DFMT_X8R8G8B8, i, &m);
        if (m.Width == current.Width && m.Height == current.Height)
        {
            found = true; break;
        }
    }
    EXPECT_TRUE(found) << "Current display mode must appear in EnumAdapterModes list";

    // Out-of-range index must fail.
    EXPECT_NE(d3d9->EnumAdapterModes(0, D3DFMT_X8R8G8B8, count, &mode), S_OK);

    d3d9->Release();
}

TEST(D3D9Create, EnumAdapterModes_AllModesWellFormed)
{
    IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ASSERT_NE(d3d9, nullptr);

    UINT count = d3d9->GetAdapterModeCount(0, D3DFMT_X8R8G8B8);
    ASSERT_GE(count, 2u);

    for (UINT i = 0; i < count; ++i)
    {
        D3DDISPLAYMODE m = {};
        ASSERT_EQ(d3d9->EnumAdapterModes(0, D3DFMT_X8R8G8B8, i, &m), S_OK);
        EXPECT_GT(m.Width,       0u) << "Mode " << i << " width must be positive";
        EXPECT_GT(m.Height,      0u) << "Mode " << i << " height must be positive";
        EXPECT_GT(m.RefreshRate, 0u) << "Mode " << i << " refresh must be positive";
        EXPECT_EQ(m.Format, D3DFMT_X8R8G8B8) << "Mode " << i << " format must match query";
    }

    d3d9->Release();
}
