#include <gtest/gtest.h>
#include <d3d11_4.h>

struct CheckFeatureSupportTests : ::testing::Test
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

TEST_F(CheckFeatureSupportTests, NullDataRejected)
{
    EXPECT_EQ(device->CheckFeatureSupport(D3D11_FEATURE_THREADING, nullptr, sizeof(D3D11_FEATURE_DATA_THREADING)), E_INVALIDARG);
}

TEST_F(CheckFeatureSupportTests, WrongSizeRejected)
{
    D3D11_FEATURE_DATA_THREADING data = {};
    EXPECT_EQ(device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &data, 1), E_INVALIDARG);
}

TEST_F(CheckFeatureSupportTests, Threading)
{
    D3D11_FEATURE_DATA_THREADING data = {};
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_FALSE(data.DriverConcurrentCreates);
    EXPECT_FALSE(data.DriverCommandLists);
}

TEST_F(CheckFeatureSupportTests, Doubles)
{
    D3D11_FEATURE_DATA_DOUBLES data = {};
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_DOUBLES, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_FALSE(data.DoublePrecisionFloatShaderOps);
}

TEST_F(CheckFeatureSupportTests, FormatSupport)
{
    D3D11_FEATURE_DATA_FORMAT_SUPPORT data = {};
    data.InFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_TRUE(data.OutFormatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D);
    EXPECT_TRUE(data.OutFormatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET);
}

TEST_F(CheckFeatureSupportTests, FormatSupport2)
{
    D3D11_FEATURE_DATA_FORMAT_SUPPORT2 data = {};
    data.InFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_EQ(data.OutFormatSupport2, 0u);
}

TEST_F(CheckFeatureSupportTests, D3D11Options)
{
    D3D11_FEATURE_DATA_D3D11_OPTIONS data = {};
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_FALSE(data.OutputMergerLogicOp);
}

TEST_F(CheckFeatureSupportTests, ArchitectureInfo)
{
    D3D11_FEATURE_DATA_ARCHITECTURE_INFO data = {};
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_ARCHITECTURE_INFO, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_FALSE(data.TileBasedDeferredRenderer);
}

TEST_F(CheckFeatureSupportTests, ShaderMinPrecision)
{
    D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT data = {};
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_EQ(data.PixelShaderMinPrecision, 0u);
    EXPECT_EQ(data.AllOtherShaderStagesMinPrecision, 0u);
}

TEST_F(CheckFeatureSupportTests, D3D9Options)
{
    D3D11_FEATURE_DATA_D3D9_OPTIONS data = {};
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D9_OPTIONS, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_TRUE(data.FullNonPow2TextureSupport);
}

TEST_F(CheckFeatureSupportTests, D3D9ShadowSupport)
{
    D3D11_FEATURE_DATA_D3D9_SHADOW_SUPPORT data = {};
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D9_SHADOW_SUPPORT, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_TRUE(data.SupportsDepthAsTextureWithLessEqualComparisonFilter);
}

TEST_F(CheckFeatureSupportTests, D3D11Options1)
{
    D3D11_FEATURE_DATA_D3D11_OPTIONS1 data = {};
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_EQ(data.TiledResourcesTier, D3D11_TILED_RESOURCES_NOT_SUPPORTED);
}

TEST_F(CheckFeatureSupportTests, D3D11Options2)
{
    D3D11_FEATURE_DATA_D3D11_OPTIONS2 data = {};
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_FALSE(data.TypedUAVLoadAdditionalFormats);
}

TEST_F(CheckFeatureSupportTests, D3D11Options3)
{
    D3D11_FEATURE_DATA_D3D11_OPTIONS3 data = {};
    HRESULT hr = device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &data, sizeof(data));
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_FALSE(data.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer);
}

TEST_F(CheckFeatureSupportTests, UnsupportedFeatureReturnsInvalidArg)
{
    char data[64] = {};
    HRESULT hr = device->CheckFeatureSupport(static_cast<D3D11_FEATURE>(9999), data, sizeof(data));
    EXPECT_EQ(hr, E_INVALIDARG);
}
