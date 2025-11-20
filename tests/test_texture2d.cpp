#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "resources/texture2d.h"

using namespace d3d11sw;

struct Texture2DTests : ::testing::Test
{
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* context = nullptr;

    void SetUp() override
    {
        D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &device, &featureLevel, &context);
        ASSERT_TRUE(SUCCEEDED(hr));
    }

    void TearDown() override
    {
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }
};

TEST_F(Texture2DTests, BasicCreation)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width          = 64;
    desc.Height         = 64;
    desc.MipLevels      = 1;
    desc.ArraySize      = 1;
    desc.Format         = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc     = { 1, 0 };
    desc.Usage          = D3D11_USAGE_DEFAULT;
    desc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, nullptr, &tex);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(tex, nullptr);
    tex->Release();
}

TEST_F(Texture2DTests, NullDescRejected)
{
    ID3D11Texture2D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture2D(nullptr, nullptr, &tex)));
}

TEST_F(Texture2DTests, ImmutableWithoutInitialDataRejected)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 64;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture2D(&desc, nullptr, &tex)));
}

TEST_F(Texture2DTests, ImmutableWithInitialDataAllowed)
{
    constexpr UINT width = 4, height = 4;
    UINT8 data[width * height * 4] = {};

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = width;
    desc.Height     = height;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem     = data;
    initData.SysMemPitch = width * 4;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, &initData, &tex)));
    tex->Release();
}

TEST_F(Texture2DTests, NullOutputReturnsSFalse)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 64;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;

    EXPECT_EQ(device->CreateTexture2D(&desc, nullptr, nullptr), S_FALSE);
}

TEST_F(Texture2DTests, DescRoundtrip)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width          = 128;
    desc.Height         = 64;
    desc.MipLevels      = 3;
    desc.ArraySize      = 2;
    desc.Format         = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc     = { 1, 0 };
    desc.Usage          = D3D11_USAGE_DEFAULT;
    desc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags      = 0;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &tex)));

    D3D11_TEXTURE2D_DESC out = {};
    tex->GetDesc(&out);
    EXPECT_EQ(out.Width,     128u);
    EXPECT_EQ(out.Height,    64u);
    EXPECT_EQ(out.MipLevels, 3u);
    EXPECT_EQ(out.ArraySize, 2u);
    EXPECT_EQ(out.Format,    DXGI_FORMAT_R8G8B8A8_UNORM);

    tex->Release();
}

TEST_F(Texture2DTests, MipLevelsAutoComputedSquare)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 32;
    desc.Height     = 32;
    desc.MipLevels  = 0; // auto
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &tex)));

    D3D11_TEXTURE2D_DESC out = {};
    tex->GetDesc(&out);
    EXPECT_EQ(out.MipLevels, 6u); // 32->16->8->4->2->1

    tex->Release();
}

TEST_F(Texture2DTests, MipLevelsAutoComputedNonSquare)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 16;
    desc.MipLevels  = 0; // auto — driven by max(64,16)=64
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &tex)));

    D3D11_TEXTURE2D_DESC out = {};
    tex->GetDesc(&out);
    EXPECT_EQ(out.MipLevels, 7u); // 64->32->16->8->4->2->1

    tex->Release();
}

TEST_F(Texture2DTests, SubresourceCount)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 64;
    desc.MipLevels  = 3;
    desc.ArraySize  = 4;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &tex)));

    EXPECT_EQ(static_cast<D3D11Texture2DSW*>(tex)->GetSubresourceCount(), 3u * 4u);

    tex->Release();
}

TEST_F(Texture2DTests, LayoutMip0Slice0)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 32;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM; // 4 bytes/pixel
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &tex)));

    auto layout = static_cast<D3D11Texture2DSW*>(tex)->GetSubresourceLayout(0);
    EXPECT_EQ(layout.Offset,     0ull);
    EXPECT_EQ(layout.RowPitch,   64u * 4u);
    EXPECT_EQ(layout.DepthPitch, 64u * 4u * 32u);
    EXPECT_EQ(layout.NumRows,    32u);
    EXPECT_EQ(layout.NumSlices,  1u);

    tex->Release();
}

TEST_F(Texture2DTests, InitialDataCopied)
{
    constexpr UINT width = 4, height = 2;
    // 4 pixels wide * 2 rows * 4 bytes/pixel
    UINT8 data[width * height * 4];
    for (UINT i = 0; i < sizeof(data); ++i) data[i] = (UINT8)(i + 1);

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = width;
    desc.Height     = height;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem    = data;
    initData.SysMemPitch = width * 4;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, &initData, &tex)));

    auto* sw  = static_cast<D3D11Texture2DSW*>(tex);
    const UINT8* ptr = static_cast<const UINT8*>(sw->GetDataPtr());
    for (UINT i = 0; i < sizeof(data); ++i)
        EXPECT_EQ(ptr[i], data[i]) << "mismatch at byte " << i;

    tex->Release();
}

TEST_F(Texture2DTests, MultisampledWithInitialDataRejected)
{
    UINT8 data[64 * 64 * 4] = {};

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 64;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 4, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_RENDER_TARGET;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem     = data;
    initData.SysMemPitch = 64 * 4;

    ID3D11Texture2D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture2D(&desc, &initData, &tex)));
}

TEST_F(Texture2DTests, MultisampledWithoutInitialDataAllowed)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 64;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 4, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &tex)));
    tex->Release();
}

// ---- CreateTexture2D1 ----

TEST_F(Texture2DTests, CreateTexture2D1Basic)
{
    ID3D11Device3* device3 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device3), (void**)&device3)));

    D3D11_TEXTURE2D_DESC1 desc = {};
    desc.Width         = 32;
    desc.Height        = 32;
    desc.MipLevels     = 1;
    desc.ArraySize     = 1;
    desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc    = { 1, 0 };
    desc.Usage         = D3D11_USAGE_DEFAULT;
    desc.BindFlags     = D3D11_BIND_SHADER_RESOURCE;
    desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    ID3D11Texture2D1* tex = nullptr;
    HRESULT hr = device3->CreateTexture2D1(&desc, nullptr, &tex);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(tex, nullptr);

    D3D11_TEXTURE2D_DESC1 out = {};
    tex->GetDesc1(&out);
    EXPECT_EQ(out.Width,  32u);
    EXPECT_EQ(out.Height, 32u);
    EXPECT_EQ(out.TextureLayout, D3D11_TEXTURE_LAYOUT_UNDEFINED);

    tex->Release();
    device3->Release();
}

TEST_F(Texture2DTests, CreateTexture2D1NullOutputReturnsSFalse)
{
    ID3D11Device3* device3 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device3), (void**)&device3)));

    D3D11_TEXTURE2D_DESC1 desc = {};
    desc.Width         = 16;
    desc.Height        = 16;
    desc.MipLevels     = 1;
    desc.ArraySize     = 1;
    desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc    = { 1, 0 };
    desc.Usage         = D3D11_USAGE_DEFAULT;
    desc.BindFlags     = D3D11_BIND_SHADER_RESOURCE;
    desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    EXPECT_EQ(device3->CreateTexture2D1(&desc, nullptr, nullptr), S_FALSE);
    device3->Release();
}

TEST_F(Texture2DTests, CreateTexture2D1ImmutableWithoutInitialDataRejected)
{
    ID3D11Device3* device3 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device3), (void**)&device3)));

    D3D11_TEXTURE2D_DESC1 desc = {};
    desc.Width         = 16;
    desc.Height        = 16;
    desc.MipLevels     = 1;
    desc.ArraySize     = 1;
    desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc    = { 1, 0 };
    desc.Usage         = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags     = D3D11_BIND_SHADER_RESOURCE;
    desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    ID3D11Texture2D1* tex = nullptr;
    EXPECT_TRUE(FAILED(device3->CreateTexture2D1(&desc, nullptr, &tex)));
    device3->Release();
}

TEST_F(Texture2DTests, CreateTexture2D1ImmutableWithInitialDataAllowed)
{
    ID3D11Device3* device3 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device3), (void**)&device3)));

    constexpr UINT width = 4, height = 4;
    UINT8 data[width * height * 4] = {};

    D3D11_TEXTURE2D_DESC1 desc = {};
    desc.Width         = width;
    desc.Height        = height;
    desc.MipLevels     = 1;
    desc.ArraySize     = 1;
    desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc    = { 1, 0 };
    desc.Usage         = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags     = D3D11_BIND_SHADER_RESOURCE;
    desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem     = data;
    initData.SysMemPitch = width * 4;

    ID3D11Texture2D1* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device3->CreateTexture2D1(&desc, &initData, &tex)));
    tex->Release();
    device3->Release();
}

TEST_F(Texture2DTests, CreateTexture2D1MultisampledWithInitialDataRejected)
{
    ID3D11Device3* device3 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device3), (void**)&device3)));

    UINT8 data[64 * 64 * 4] = {};

    D3D11_TEXTURE2D_DESC1 desc = {};
    desc.Width         = 64;
    desc.Height        = 64;
    desc.MipLevels     = 1;
    desc.ArraySize     = 1;
    desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc    = { 4, 0 };
    desc.Usage         = D3D11_USAGE_DEFAULT;
    desc.BindFlags     = D3D11_BIND_RENDER_TARGET;
    desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem     = data;
    initData.SysMemPitch = 64 * 4;

    ID3D11Texture2D1* tex = nullptr;
    EXPECT_TRUE(FAILED(device3->CreateTexture2D1(&desc, &initData, &tex)));
    device3->Release();
}

TEST_F(Texture2DTests, UnknownFormatRejected)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 64;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture2D(&desc, nullptr, &tex)));
}

TEST_F(Texture2DTests, DepthFormatWithRenderTargetRejected)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 64;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_D32_FLOAT;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture2D(&desc, nullptr, &tex)));
}

TEST_F(Texture2DTests, ColorFormatWithDepthStencilRejected)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 64;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture2D(&desc, nullptr, &tex)));
}

TEST_F(Texture2DTests, DepthFormatWithDepthStencilAllowed)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 64;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_D32_FLOAT;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &tex)));
    tex->Release();
}

TEST_F(Texture2DTests, BCFormatWithRenderTargetRejected)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = 64;
    desc.Height     = 64;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_BC1_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage      = D3D11_USAGE_DEFAULT;
    desc.BindFlags  = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture2D(&desc, nullptr, &tex)));
}
