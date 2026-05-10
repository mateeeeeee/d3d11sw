#include <gtest/gtest.h>
#include <d3d11_4.h>

struct DeviceCreationTests : ::testing::Test
{
    ID3D11Device*        device       = nullptr;
    ID3D11DeviceContext* context      = nullptr;
    D3D_FEATURE_LEVEL    featureLevel = {};

    void SetUp() override
    {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &device,
            &featureLevel,
            &context);
        ASSERT_TRUE(SUCCEEDED(hr));
        ASSERT_NE(device, nullptr);
        ASSERT_NE(context, nullptr);
    }

    void TearDown() override
    {
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }
};

TEST_F(DeviceCreationTests, DeviceAndContextCreated)
{
    EXPECT_NE(device, nullptr);
    EXPECT_NE(context, nullptr);
    EXPECT_NE(featureLevel, 0u);
}

TEST_F(DeviceCreationTests, QueryDevice1)
{
    ID3D11Device1* device1 = nullptr;
    HRESULT hr = device->QueryInterface(__uuidof(ID3D11Device1), (void**)&device1);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(device1, nullptr);
    device1->Release();
}
