#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "resources/texture3d.h"

using namespace d3d11sw;

struct Texture3DTests : ::testing::Test
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

TEST_F(Texture3DTests, BasicCreation)
{
    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = 16;
    desc.Height    = 16;
    desc.Depth     = 4;
    desc.MipLevels = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture3D* tex = nullptr;
    HRESULT hr = device->CreateTexture3D(&desc, nullptr, &tex);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(tex, nullptr);
    tex->Release();
}

TEST_F(Texture3DTests, NullDescRejected)
{
    ID3D11Texture3D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture3D(nullptr, nullptr, &tex)));
}

TEST_F(Texture3DTests, NullOutputReturnsSFalse)
{
    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = 8;
    desc.Height    = 8;
    desc.Depth     = 2;
    desc.MipLevels = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    EXPECT_EQ(device->CreateTexture3D(&desc, nullptr, nullptr), S_FALSE);
}

TEST_F(Texture3DTests, ImmutableWithoutInitialDataRejected)
{
    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = 8;
    desc.Height    = 8;
    desc.Depth     = 4;
    desc.MipLevels = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture3D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture3D(&desc, nullptr, &tex)));
}

TEST_F(Texture3DTests, ImmutableWithInitialDataAllowed)
{
    constexpr UINT width = 4, height = 4, depth = 2;
    UINT8 data[width * height * depth * 4] = {};

    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = width;
    desc.Height    = height;
    desc.Depth     = depth;
    desc.MipLevels = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem          = data;
    initData.SysMemPitch      = width * 4;
    initData.SysMemSlicePitch = width * height * 4;

    ID3D11Texture3D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture3D(&desc, &initData, &tex)));
    tex->Release();
}

TEST_F(Texture3DTests, DescRoundtrip)
{
    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width          = 32;
    desc.Height         = 16;
    desc.Depth          = 8;
    desc.MipLevels      = 2;
    desc.Format         = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage          = D3D11_USAGE_DEFAULT;
    desc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags      = 0;

    ID3D11Texture3D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture3D(&desc, nullptr, &tex)));

    D3D11_TEXTURE3D_DESC out = {};
    tex->GetDesc(&out);
    EXPECT_EQ(out.Width,     32u);
    EXPECT_EQ(out.Height,    16u);
    EXPECT_EQ(out.Depth,     8u);
    EXPECT_EQ(out.MipLevels, 2u);
    EXPECT_EQ(out.Format,    DXGI_FORMAT_R8G8B8A8_UNORM);

    tex->Release();
}

TEST_F(Texture3DTests, MipLevelsAutoComputed)
{
    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = 32;
    desc.Height    = 16;
    desc.Depth     = 8;
    desc.MipLevels = 0; // auto — driven by max(32,16,8)=32
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture3D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture3D(&desc, nullptr, &tex)));

    D3D11_TEXTURE3D_DESC out = {};
    tex->GetDesc(&out);
    EXPECT_EQ(out.MipLevels, 6u); // 32->16->8->4->2->1

    tex->Release();
}

TEST_F(Texture3DTests, SubresourceCount)
{
    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = 16;
    desc.Height    = 16;
    desc.Depth     = 8;
    desc.MipLevels = 4;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture3D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture3D(&desc, nullptr, &tex)));

    // 3D textures: arraySize=1, so subresource count == MipLevels
    EXPECT_EQ(static_cast<D3D11Texture3DSW*>(tex)->GetSubresourceCount(), 4u);

    tex->Release();
}

TEST_F(Texture3DTests, LayoutMip0)
{
    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = 8;
    desc.Height    = 4;
    desc.Depth     = 2;
    desc.MipLevels = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM; // 4 bytes/pixel
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture3D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture3D(&desc, nullptr, &tex)));

    auto layout = static_cast<D3D11Texture3DSW*>(tex)->GetSubresourceLayout(0);
    EXPECT_EQ(layout.Offset,     0ull);
    EXPECT_EQ(layout.RowPitch,   8u * 4u);          // 8 pixels * 4 bytes
    EXPECT_EQ(layout.DepthPitch, 8u * 4u * 4u);     // rowPitch * 4 rows
    EXPECT_EQ(layout.NumRows,    4u);
    EXPECT_EQ(layout.NumSlices,  2u);

    tex->Release();
}

TEST_F(Texture3DTests, InitialDataCopied)
{
    constexpr UINT width = 2, height = 2, depth = 2;
    UINT8 data[width * height * depth * 4];
    for (UINT i = 0; i < sizeof(data); ++i) data[i] = (UINT8)(i + 1);

    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = width;
    desc.Height    = height;
    desc.Depth     = depth;
    desc.MipLevels = 1;
    desc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem          = data;
    initData.SysMemPitch      = width * 4;
    initData.SysMemSlicePitch = width * height * 4;

    ID3D11Texture3D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture3D(&desc, &initData, &tex)));

    auto* sw = static_cast<D3D11Texture3DSW*>(tex);
    const UINT8* ptr = static_cast<const UINT8*>(sw->GetDataPtr());
    for (UINT i = 0; i < sizeof(data); ++i)
        EXPECT_EQ(ptr[i], data[i]) << "mismatch at byte " << i;

    tex->Release();
}

// ---- CreateTexture3D1 ----

TEST_F(Texture3DTests, CreateTexture3D1Basic)
{
    ID3D11Device3* device3 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device3), (void**)&device3)));

    D3D11_TEXTURE3D_DESC1 desc = {};
    desc.Width         = 16;
    desc.Height        = 16;
    desc.Depth         = 4;
    desc.MipLevels     = 1;
    desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage         = D3D11_USAGE_DEFAULT;
    desc.BindFlags     = D3D11_BIND_SHADER_RESOURCE;
    desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    ID3D11Texture3D1* tex = nullptr;
    HRESULT hr = device3->CreateTexture3D1(&desc, nullptr, &tex);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(tex, nullptr);

    D3D11_TEXTURE3D_DESC1 out = {};
    tex->GetDesc1(&out);
    EXPECT_EQ(out.Width,  16u);
    EXPECT_EQ(out.Height, 16u);
    EXPECT_EQ(out.Depth,  4u);
    EXPECT_EQ(out.TextureLayout, D3D11_TEXTURE_LAYOUT_UNDEFINED);

    tex->Release();
    device3->Release();
}

TEST_F(Texture3DTests, CreateTexture3D1NullOutputReturnsSFalse)
{
    ID3D11Device3* device3 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device3), (void**)&device3)));

    D3D11_TEXTURE3D_DESC1 desc = {};
    desc.Width         = 8;
    desc.Height        = 8;
    desc.Depth         = 2;
    desc.MipLevels     = 1;
    desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage         = D3D11_USAGE_DEFAULT;
    desc.BindFlags     = D3D11_BIND_SHADER_RESOURCE;
    desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    EXPECT_EQ(device3->CreateTexture3D1(&desc, nullptr, nullptr), S_FALSE);
    device3->Release();
}

TEST_F(Texture3DTests, CreateTexture3D1ImmutableWithoutInitialDataRejected)
{
    ID3D11Device3* device3 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device3), (void**)&device3)));

    D3D11_TEXTURE3D_DESC1 desc = {};
    desc.Width         = 8;
    desc.Height        = 8;
    desc.Depth         = 2;
    desc.MipLevels     = 1;
    desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage         = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags     = D3D11_BIND_SHADER_RESOURCE;
    desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    ID3D11Texture3D1* tex = nullptr;
    EXPECT_TRUE(FAILED(device3->CreateTexture3D1(&desc, nullptr, &tex)));
    device3->Release();
}

TEST_F(Texture3DTests, CreateTexture3D1ImmutableWithInitialDataAllowed)
{
    ID3D11Device3* device3 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device3), (void**)&device3)));

    constexpr UINT width = 4, height = 4, depth = 2;
    UINT8 data[width * height * depth * 4] = {};

    D3D11_TEXTURE3D_DESC1 desc = {};
    desc.Width         = width;
    desc.Height        = height;
    desc.Depth         = depth;
    desc.MipLevels     = 1;
    desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage         = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags     = D3D11_BIND_SHADER_RESOURCE;
    desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem          = data;
    initData.SysMemPitch      = width * 4;
    initData.SysMemSlicePitch = width * height * 4;

    ID3D11Texture3D1* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device3->CreateTexture3D1(&desc, &initData, &tex)));
    tex->Release();
    device3->Release();
}

TEST_F(Texture3DTests, UnknownFormatRejected)
{
    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = 8;
    desc.Height    = 8;
    desc.Depth     = 2;
    desc.MipLevels = 1;
    desc.Format    = DXGI_FORMAT_UNKNOWN;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture3D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture3D(&desc, nullptr, &tex)));
}

TEST_F(Texture3DTests, DepthFormatWithDepthStencilRejected)
{
    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = 8;
    desc.Height    = 8;
    desc.Depth     = 2;
    desc.MipLevels = 1;
    desc.Format    = DXGI_FORMAT_D32_FLOAT;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL; // never valid for 3D

    ID3D11Texture3D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture3D(&desc, nullptr, &tex)));
}

TEST_F(Texture3DTests, DepthFormatWithRenderTargetRejected)
{
    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = 8;
    desc.Height    = 8;
    desc.Depth     = 2;
    desc.MipLevels = 1;
    desc.Format    = DXGI_FORMAT_D32_FLOAT;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture3D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture3D(&desc, nullptr, &tex)));
}

TEST_F(Texture3DTests, BCFormatWithRenderTargetRejected)
{
    D3D11_TEXTURE3D_DESC desc = {};
    desc.Width     = 8;
    desc.Height    = 8;
    desc.Depth     = 2;
    desc.MipLevels = 1;
    desc.Format    = DXGI_FORMAT_BC1_UNORM;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture3D* tex = nullptr;
    EXPECT_TRUE(FAILED(device->CreateTexture3D(&desc, nullptr, &tex)));
}
