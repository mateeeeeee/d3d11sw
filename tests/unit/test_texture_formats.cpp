#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <cstring>
#include "resources/texture2d.h"

using namespace d3d11sw;

struct TextureFormatTests : ::testing::Test
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
        ASSERT_NE(device, nullptr);
        ASSERT_NE(context, nullptr);
    }

    void TearDown() override
    {
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }

    ID3D11Texture2D* CreateTex(DXGI_FORMAT fmt, UINT w, UINT h, UINT bindFlags,
                                const void* data = nullptr, UINT pitch = 0)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width            = w;
        desc.Height           = h;
        desc.MipLevels        = 1;
        desc.ArraySize        = 1;
        desc.Format           = fmt;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_DEFAULT;
        desc.BindFlags        = bindFlags;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem     = data;
        initData.SysMemPitch = pitch;

        ID3D11Texture2D* tex = nullptr;
        device->CreateTexture2D(&desc, data ? &initData : nullptr, &tex);
        return tex;
    }
};

TEST_F(TextureFormatTests, R8G8B8A8_UNORM_Creation)
{
    ID3D11Texture2D* tex = CreateTex(DXGI_FORMAT_R8G8B8A8_UNORM, 32, 32, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);
    EXPECT_EQ(desc.Format, DXGI_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(desc.Width, 32u);
    EXPECT_EQ(desc.Height, 32u);

    tex->Release();
}

TEST_F(TextureFormatTests, B8G8R8A8_UNORM_Creation)
{
    ID3D11Texture2D* tex = CreateTex(DXGI_FORMAT_B8G8R8A8_UNORM, 16, 16, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);
    EXPECT_EQ(desc.Format, DXGI_FORMAT_B8G8R8A8_UNORM);

    tex->Release();
}

TEST_F(TextureFormatTests, R32_FLOAT_Creation)
{
    ID3D11Texture2D* tex = CreateTex(DXGI_FORMAT_R32_FLOAT, 8, 8, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);
    EXPECT_EQ(desc.Format, DXGI_FORMAT_R32_FLOAT);

    tex->Release();
}

TEST_F(TextureFormatTests, R32G32B32A32_FLOAT_Creation)
{
    ID3D11Texture2D* tex = CreateTex(DXGI_FORMAT_R32G32B32A32_FLOAT, 4, 4, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);
    EXPECT_EQ(desc.Format, DXGI_FORMAT_R32G32B32A32_FLOAT);

    tex->Release();
}

TEST_F(TextureFormatTests, R16G16B16A16_UNORM_Creation)
{
    ID3D11Texture2D* tex = CreateTex(DXGI_FORMAT_R16G16B16A16_UNORM, 8, 8, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);
    EXPECT_EQ(desc.Format, DXGI_FORMAT_R16G16B16A16_UNORM);

    tex->Release();
}

TEST_F(TextureFormatTests, R32_UINT_Creation)
{
    ID3D11Texture2D* tex = CreateTex(DXGI_FORMAT_R32_UINT, 16, 16, D3D11_BIND_UNORDERED_ACCESS);
    ASSERT_NE(tex, nullptr);

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);
    EXPECT_EQ(desc.Format, DXGI_FORMAT_R32_UINT);

    tex->Release();
}

TEST_F(TextureFormatTests, D32_FLOAT_Creation)
{
    ID3D11Texture2D* tex = CreateTex(DXGI_FORMAT_D32_FLOAT, 64, 64, D3D11_BIND_DEPTH_STENCIL);
    ASSERT_NE(tex, nullptr);

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);
    EXPECT_EQ(desc.Format, DXGI_FORMAT_D32_FLOAT);

    tex->Release();
}

TEST_F(TextureFormatTests, D16_UNORM_Creation)
{
    ID3D11Texture2D* tex = CreateTex(DXGI_FORMAT_D16_UNORM, 64, 64, D3D11_BIND_DEPTH_STENCIL);
    ASSERT_NE(tex, nullptr);

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);
    EXPECT_EQ(desc.Format, DXGI_FORMAT_D16_UNORM);

    tex->Release();
}

TEST_F(TextureFormatTests, R8G8B8A8_UNORM_DataLayout)
{
    UINT8 pixels[4 * 4 * 4];
    for (int i = 0; i < 4 * 4; ++i)
    {
        pixels[i * 4 + 0] = 255; // R
        pixels[i * 4 + 1] = 0;   // G
        pixels[i * 4 + 2] = 128; // B
        pixels[i * 4 + 3] = 255; // A
    }

    ID3D11Texture2D* tex = CreateTex(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4,
                                      D3D11_BIND_RENDER_TARGET, pixels, 4 * 4);
    ASSERT_NE(tex, nullptr);

    auto* sw = static_cast<D3D11Texture2DSW*>(tex);
    auto layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    EXPECT_EQ(data[0], 255u); // R
    EXPECT_EQ(data[1], 0u);   // G
    EXPECT_EQ(data[2], 128u); // B
    EXPECT_EQ(data[3], 255u); // A

    tex->Release();
}

TEST_F(TextureFormatTests, R32_FLOAT_DataLayout)
{
    float pixels[4 * 4] = {};
    pixels[0] = 3.14f;
    pixels[1] = 2.72f;

    ID3D11Texture2D* tex = CreateTex(DXGI_FORMAT_R32_FLOAT, 4, 4,
                                      D3D11_BIND_RENDER_TARGET, pixels, 4 * sizeof(float));
    ASSERT_NE(tex, nullptr);

    auto* sw = static_cast<D3D11Texture2DSW*>(tex);
    auto layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    float val0, val1;
    std::memcpy(&val0, data, 4);
    std::memcpy(&val1, data + 4, 4);
    EXPECT_FLOAT_EQ(val0, 3.14f);
    EXPECT_FLOAT_EQ(val1, 2.72f);

    tex->Release();
}

TEST_F(TextureFormatTests, B8G8R8A8_UNORM_DataLayout)
{
    UINT8 pixels[4] = {0, 128, 255, 255}; // B=0, G=128, R=255, A=255

    ID3D11Texture2D* tex = CreateTex(DXGI_FORMAT_B8G8R8A8_UNORM, 1, 1,
                                      D3D11_BIND_RENDER_TARGET, pixels, 4);
    ASSERT_NE(tex, nullptr);

    auto* sw = static_cast<D3D11Texture2DSW*>(tex);
    auto layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    EXPECT_EQ(data[0], 0u);   // B
    EXPECT_EQ(data[1], 128u); // G
    EXPECT_EQ(data[2], 255u); // R
    EXPECT_EQ(data[3], 255u); // A

    tex->Release();
}

TEST_F(TextureFormatTests, Staging_R8G8B8A8_MapReadWrite)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width            = 4;
    desc.Height           = 4;
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags   = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &tex)));

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(tex, 0, D3D11_MAP_WRITE, 0, &mapped)));

    UINT8* ptr = static_cast<UINT8*>(mapped.pData);
    for (UINT y = 0; y < 4; ++y)
    {
        UINT8* row = ptr + y * mapped.RowPitch;
        for (UINT x = 0; x < 4; ++x)
        {
            row[x * 4 + 0] = static_cast<UINT8>(y * 4 + x);
            row[x * 4 + 1] = 0;
            row[x * 4 + 2] = 0;
            row[x * 4 + 3] = 255;
        }
    }
    context->Unmap(tex, 0);

    ASSERT_TRUE(SUCCEEDED(context->Map(tex, 0, D3D11_MAP_READ, 0, &mapped)));
    ptr = static_cast<UINT8*>(mapped.pData);
    for (UINT y = 0; y < 4; ++y)
    {
        const UINT8* row = ptr + y * mapped.RowPitch;
        for (UINT x = 0; x < 4; ++x)
        {
            EXPECT_EQ(row[x * 4 + 0], static_cast<UINT8>(y * 4 + x));
        }
    }
    context->Unmap(tex, 0);

    tex->Release();
}

TEST_F(TextureFormatTests, FormatSupport_R8G8B8A8_UNORM)
{
    UINT support = 0;
    HRESULT hr = device->CheckFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM, &support);
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);
}

TEST_F(TextureFormatTests, FormatSupport_D32_FLOAT)
{
    UINT support = 0;
    HRESULT hr = device->CheckFormatSupport(DXGI_FORMAT_D32_FLOAT, &support);
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
}

TEST_F(TextureFormatTests, FormatSupport_R32_FLOAT)
{
    UINT support = 0;
    HRESULT hr = device->CheckFormatSupport(DXGI_FORMAT_R32_FLOAT, &support);
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
}
