#include <gtest/gtest.h>
#include <d3d11_4.h>

struct CheckFormatSupportTests : ::testing::Test
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

TEST_F(CheckFormatSupportTests, NullOutputRejected)
{
    EXPECT_TRUE(FAILED(device->CheckFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM, nullptr)));
}

TEST_F(CheckFormatSupportTests, UnknownFormatReturnsZero)
{
    UINT support = ~0u;
    HRESULT hr = device->CheckFormatSupport(DXGI_FORMAT_UNKNOWN, &support);
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_EQ(support, 0u);
}

TEST_F(CheckFormatSupportTests, ColorFormatHasTexture2DFlag)
{
    UINT support = 0;
    ASSERT_TRUE(SUCCEEDED(device->CheckFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM, &support)));
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
}

TEST_F(CheckFormatSupportTests, ColorFormatHasRenderTargetFlag)
{
    UINT support = 0;
    ASSERT_TRUE(SUCCEEDED(device->CheckFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM, &support)));
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
}

TEST_F(CheckFormatSupportTests, ColorFormatHasShaderSampleFlag)
{
    UINT support = 0;
    ASSERT_TRUE(SUCCEEDED(device->CheckFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM, &support)));
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);
}

TEST_F(CheckFormatSupportTests, DepthFormatHasDepthStencilFlag)
{
    UINT support = 0;
    ASSERT_TRUE(SUCCEEDED(device->CheckFormatSupport(DXGI_FORMAT_D32_FLOAT, &support)));
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL);
}

TEST_F(CheckFormatSupportTests, DepthFormatLacksRenderTargetFlag)
{
    UINT support = 0;
    ASSERT_TRUE(SUCCEEDED(device->CheckFormatSupport(DXGI_FORMAT_D32_FLOAT, &support)));
    EXPECT_FALSE(support & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
}

TEST_F(CheckFormatSupportTests, BCFormatHasShaderSampleFlag)
{
    UINT support = 0;
    ASSERT_TRUE(SUCCEEDED(device->CheckFormatSupport(DXGI_FORMAT_BC1_UNORM, &support)));
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE);
}

TEST_F(CheckFormatSupportTests, BCFormatLacksRenderTargetFlag)
{
    UINT support = 0;
    ASSERT_TRUE(SUCCEEDED(device->CheckFormatSupport(DXGI_FORMAT_BC1_UNORM, &support)));
    EXPECT_FALSE(support & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
}

TEST_F(CheckFormatSupportTests, TypelessFormatHasTexture2DFlag)
{
    UINT support = 0;
    ASSERT_TRUE(SUCCEEDED(device->CheckFormatSupport(DXGI_FORMAT_R32_TYPELESS, &support)));
    EXPECT_TRUE(support & D3D11_FORMAT_SUPPORT_TEXTURE2D);
}
