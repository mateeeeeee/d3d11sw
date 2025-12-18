#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "dxgi/swapchain.h"
#include "resources/texture2d.h"

using namespace d3d11sw;

struct SwapChainTests : ::testing::Test
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

    IDXGISwapChain* CreateSwapChain(UINT width, UINT height, DXGI_FORMAT format)
    {
        DXGI_SWAP_CHAIN_DESC desc{};
        desc.BufferDesc.Width  = width;
        desc.BufferDesc.Height = height;
        desc.BufferDesc.Format = format;
        desc.BufferCount       = 1;
        desc.SampleDesc        = { 1, 0 };
        desc.OutputWindow      = nullptr;
        desc.Windowed          = TRUE;
        desc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;

        auto* sc = new DXGISwapChainSW(device, desc);
        IDXGISwapChain* result = nullptr;
        sc->QueryInterface(__uuidof(IDXGISwapChain), (void**)&result);
        sc->Release();
        return result;
    }
};

TEST_F(SwapChainTests, GetBuffer_ReturnsTexture2D)
{
    auto* sc = CreateSwapChain(128, 64, DXGI_FORMAT_R8G8B8A8_UNORM);
    ASSERT_NE(sc, nullptr);

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&tex);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(tex, nullptr);

    D3D11_TEXTURE2D_DESC desc{};
    tex->GetDesc(&desc);
    EXPECT_EQ(desc.Width,  128u);
    EXPECT_EQ(desc.Height, 64u);
    EXPECT_EQ(desc.Format, DXGI_FORMAT_R8G8B8A8_UNORM);
    EXPECT_TRUE(desc.BindFlags & D3D11_BIND_RENDER_TARGET);

    tex->Release();
    sc->Release();
}

TEST_F(SwapChainTests, GetBuffer_InvalidIndex)
{
    auto* sc = CreateSwapChain(64, 64, DXGI_FORMAT_R8G8B8A8_UNORM);
    ASSERT_NE(sc, nullptr);

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = sc->GetBuffer(1, __uuidof(ID3D11Texture2D), (void**)&tex);
    EXPECT_EQ(hr, DXGI_ERROR_INVALID_CALL);
    EXPECT_EQ(tex, nullptr);

    sc->Release();
}

TEST_F(SwapChainTests, Present_NullWindow)
{
    auto* sc = CreateSwapChain(64, 64, DXGI_FORMAT_R8G8B8A8_UNORM);
    ASSERT_NE(sc, nullptr);

    HRESULT hr = sc->Present(0, 0);
    EXPECT_TRUE(SUCCEEDED(hr));

    sc->Release();
}

TEST_F(SwapChainTests, ResizeBuffers_ChangesSize)
{
    auto* sc = CreateSwapChain(64, 64, DXGI_FORMAT_R8G8B8A8_UNORM);
    ASSERT_NE(sc, nullptr);

    HRESULT hr = sc->ResizeBuffers(1, 256, 128, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    ASSERT_TRUE(SUCCEEDED(hr));

    ID3D11Texture2D* tex = nullptr;
    hr = sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&tex);
    ASSERT_TRUE(SUCCEEDED(hr));

    D3D11_TEXTURE2D_DESC desc{};
    tex->GetDesc(&desc);
    EXPECT_EQ(desc.Width,  256u);
    EXPECT_EQ(desc.Height, 128u);
    EXPECT_EQ(desc.Format, DXGI_FORMAT_B8G8R8A8_UNORM);

    tex->Release();
    sc->Release();
}
