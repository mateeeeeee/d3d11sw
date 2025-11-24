#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "resources/texture1d.h"

using namespace d3d11sw;

struct Texture1DTests : ::testing::Test
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

TEST_F(Texture1DTests, BasicCreation)
{
    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width     = 64;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture1D* tex = nullptr;
    HRESULT hr = device->CreateTexture1D(&desc, nullptr, &tex);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(tex, nullptr);
    tex->Release();
}

TEST_F(Texture1DTests, NullDescRejected)
{
    ID3D11Texture1D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture1D(nullptr, nullptr, &tex)));
}

TEST_F(Texture1DTests, ImmutableWithoutInitialDataRejected)
{
    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width     = 64;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture1D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture1D(&desc, nullptr, &tex)));
}

TEST_F(Texture1DTests, NullOutputReturnsSFalse)
{
    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width     = 64;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    EXPECT_EQ(device->CreateTexture1D(&desc, nullptr, nullptr), S_FALSE);
}

TEST_F(Texture1DTests, DescRoundtrip)
{
    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width          = 128;
    desc.MipLevels      = 3;
    desc.ArraySize      = 2;
    desc.Format         = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage          = D3D11_USAGE_DEFAULT;
    desc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags      = 0;

    ID3D11Texture1D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture1D(&desc, nullptr, &tex)));

    D3D11_TEXTURE1D_DESC out = {};
    tex->GetDesc(&out);
    EXPECT_EQ(out.Width,     128u);
    EXPECT_EQ(out.MipLevels, 3u);
    EXPECT_EQ(out.ArraySize, 2u);
    EXPECT_EQ(out.Format,    DXGI_FORMAT_R8G8B8A8_UNORM);

    tex->Release();
}

TEST_F(Texture1DTests, MipLevelsAutoComputed)
{
    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width     = 32;
    desc.MipLevels = 0; // auto
    desc.ArraySize = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture1D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture1D(&desc, nullptr, &tex)));

    D3D11_TEXTURE1D_DESC out = {};
    tex->GetDesc(&out);
    EXPECT_EQ(out.MipLevels, 6u); // 32->16->8->4->2->1

    tex->Release();
}

TEST_F(Texture1DTests, SubresourceCount)
{
    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width     = 64;
    desc.MipLevels = 3;
    desc.ArraySize = 4;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture1D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture1D(&desc, nullptr, &tex)));

    EXPECT_EQ(static_cast<D3D11Texture1DSW*>(tex)->GetSubresourceCount(), 3u * 4u);

    tex->Release();
}

TEST_F(Texture1DTests, LayoutMip0Slice0)
{
    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width     = 64;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM; // 4 bytes/pixel
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture1D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture1D(&desc, nullptr, &tex)));

    auto layout = static_cast<D3D11Texture1DSW*>(tex)->GetSubresourceLayout(0);
    EXPECT_EQ(layout.Offset,    0ull);
    EXPECT_EQ(layout.RowPitch,  64u * 4u);
    EXPECT_EQ(layout.NumRows,   1u);
    EXPECT_EQ(layout.NumSlices, 1u);

    tex->Release();
}

TEST_F(Texture1DTests, InitialDataCopied)
{
    constexpr UINT width = 4;
    UINT8 data[width * 4] = { 1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16 };

    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width     = width;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;

    ID3D11Texture1D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture1D(&desc, &initData, &tex)));

    auto* sw  = static_cast<D3D11Texture1DSW*>(tex);
    const UINT8* ptr = static_cast<const UINT8*>(sw->GetDataPtr());
    for (UINT i = 0; i < sizeof(data); ++i)
        EXPECT_EQ(ptr[i], data[i]) << "mismatch at byte " << i;

    tex->Release();
}

TEST_F(Texture1DTests, UnknownFormatRejected)
{
    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width     = 64;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format    = DXGI_FORMAT_UNKNOWN;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture1D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture1D(&desc, nullptr, &tex)));
}

TEST_F(Texture1DTests, DepthFormatWithDepthStencilRejected)
{
    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width     = 64;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format    = DXGI_FORMAT_D32_FLOAT;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL; // not allowed for 1D

    ID3D11Texture1D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture1D(&desc, nullptr, &tex)));
}

TEST_F(Texture1DTests, ColorFormatWithRenderTargetAllowed)
{
    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width     = 64;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture1D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture1D(&desc, nullptr, &tex)));
    tex->Release();
}

TEST_F(Texture1DTests, BCFormatWithRenderTargetRejected)
{
    D3D11_TEXTURE1D_DESC desc = {};
    desc.Width     = 64;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format    = DXGI_FORMAT_BC1_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture1D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture1D(&desc, nullptr, &tex)));
}
