#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "views/render_target_view.h"
#include "views/depth_stencil_view.h"
#include "views/shader_resource_view.h"
#include "views/unordered_access_view.h"

using namespace d3d11sw;

struct ViewTests : ::testing::Test
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
    }

    void TearDown() override
    {
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }

    ID3D11Texture2D* MakeTex2D(DXGI_FORMAT fmt, UINT bindFlags)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width      = 64;
        desc.Height     = 64;
        desc.MipLevels  = 1;
        desc.ArraySize  = 1;
        desc.Format     = fmt;
        desc.SampleDesc = { 1, 0 };
        desc.Usage      = D3D11_USAGE_DEFAULT;
        desc.BindFlags  = bindFlags;

        ID3D11Texture2D* tex = nullptr;
        device->CreateTexture2D(&desc, nullptr, &tex);
        return tex;
    }
};

// ---- RenderTargetView ------------------------------------------------------

TEST_F(ViewTests, RTV_Creation)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    ID3D11RenderTargetView* rtv = nullptr;
    HRESULT hr = device->CreateRenderTargetView(tex, nullptr, &rtv);
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(rtv, nullptr);

    if (rtv) rtv->Release();
    tex->Release();
}

TEST_F(ViewTests, RTV_NullResourceRejected)
{
    ID3D11RenderTargetView* rtv = nullptr;
    EXPECT_TRUE(FAILED(device->CreateRenderTargetView(nullptr, nullptr, &rtv)));
}

TEST_F(ViewTests, RTV_NullOutputReturnsSFalse)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    EXPECT_EQ(device->CreateRenderTargetView(tex, nullptr, nullptr), S_FALSE);
    tex->Release();
}

TEST_F(ViewTests, RTV_GetResourceRoundtrip)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(tex, nullptr, &rtv)));

    ID3D11Resource* got = nullptr;
    rtv->GetResource(&got);
    EXPECT_EQ(got, tex);

    if (got) got->Release();
    rtv->Release();
    tex->Release();
}

TEST_F(ViewTests, RTV_GetDescRoundtrip)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    D3D11_RENDER_TARGET_VIEW_DESC desc = {};
    desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;

    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(tex, &desc, &rtv)));

    D3D11_RENDER_TARGET_VIEW_DESC out = {};
    rtv->GetDesc(&out);
    EXPECT_EQ(out.Format,        DXGI_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(out.ViewDimension, D3D11_RTV_DIMENSION_TEXTURE2D);
    EXPECT_EQ(out.Texture2D.MipSlice, 0u);

    rtv->Release();
    tex->Release();
}

TEST_F(ViewTests, RTV_NullDescCreatesDefault)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(tex, nullptr, &rtv)));

    D3D11_RENDER_TARGET_VIEW_DESC out = {};
    rtv->GetDesc(&out);
    EXPECT_EQ(out.Format,        DXGI_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(out.ViewDimension, D3D11_RTV_DIMENSION_TEXTURE2D);

    rtv->Release();
    tex->Release();
}

TEST_F(ViewTests, RTV_DataPtrNotNull)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(tex, nullptr, &rtv)));

    auto* sw = static_cast<D3D11RenderTargetViewSW*>(rtv);
    EXPECT_NE(sw->GetDataPtr(), nullptr);
    EXPECT_GT(sw->GetLayout().RowPitch, 0u);

    rtv->Release();
    tex->Release();
}

// ---- DepthStencilView -------------------------------------------------------

TEST_F(ViewTests, DSV_Creation)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL);
    ASSERT_NE(tex, nullptr);

    ID3D11DepthStencilView* dsv = nullptr;
    HRESULT hr = device->CreateDepthStencilView(tex, nullptr, &dsv);
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(dsv, nullptr);

    if (dsv) dsv->Release();
    tex->Release();
}

TEST_F(ViewTests, DSV_NullResourceRejected)
{
    ID3D11DepthStencilView* dsv = nullptr;
    EXPECT_TRUE(FAILED(device->CreateDepthStencilView(nullptr, nullptr, &dsv)));
}

TEST_F(ViewTests, DSV_NullOutputReturnsSFalse)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL);
    ASSERT_NE(tex, nullptr);

    EXPECT_EQ(device->CreateDepthStencilView(tex, nullptr, nullptr), S_FALSE);
    tex->Release();
}

TEST_F(ViewTests, DSV_GetDescRoundtrip)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL);
    ASSERT_NE(tex, nullptr);

    D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
    desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;

    ID3D11DepthStencilView* dsv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilView(tex, &desc, &dsv)));

    D3D11_DEPTH_STENCIL_VIEW_DESC out = {};
    dsv->GetDesc(&out);
    EXPECT_EQ(out.Format,             DXGI_FORMAT_D24_UNORM_S8_UINT);
    EXPECT_EQ(out.ViewDimension,      D3D11_DSV_DIMENSION_TEXTURE2D);
    EXPECT_EQ(out.Texture2D.MipSlice, 0u);

    dsv->Release();
    tex->Release();
}

// ---- ShaderResourceView ----------------------------------------------------

TEST_F(ViewTests, SRV_Creation)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_SHADER_RESOURCE);
    ASSERT_NE(tex, nullptr);

    ID3D11ShaderResourceView* srv = nullptr;
    HRESULT hr = device->CreateShaderResourceView(tex, nullptr, &srv);
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(srv, nullptr);

    if (srv) srv->Release();
    tex->Release();
}

TEST_F(ViewTests, SRV_NullResourceRejected)
{
    ID3D11ShaderResourceView* srv = nullptr;
    EXPECT_TRUE(FAILED(device->CreateShaderResourceView(nullptr, nullptr, &srv)));
}

TEST_F(ViewTests, SRV_NullOutputReturnsSFalse)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_SHADER_RESOURCE);
    ASSERT_NE(tex, nullptr);

    EXPECT_EQ(device->CreateShaderResourceView(tex, nullptr, nullptr), S_FALSE);
    tex->Release();
}

TEST_F(ViewTests, SRV_GetDescRoundtrip)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_SHADER_RESOURCE);
    ASSERT_NE(tex, nullptr);

    D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MostDetailedMip = 0;
    desc.Texture2D.MipLevels       = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(tex, &desc, &srv)));

    D3D11_SHADER_RESOURCE_VIEW_DESC out = {};
    srv->GetDesc(&out);
    EXPECT_EQ(out.Format,                    DXGI_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(out.ViewDimension,             D3D11_SRV_DIMENSION_TEXTURE2D);
    EXPECT_EQ(out.Texture2D.MostDetailedMip, 0u);
    EXPECT_EQ(out.Texture2D.MipLevels,       1u);

    srv->Release();
    tex->Release();
}

// ---- UnorderedAccessView ---------------------------------------------------

TEST_F(ViewTests, UAV_Creation)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_UNORDERED_ACCESS);
    ASSERT_NE(tex, nullptr);

    ID3D11UnorderedAccessView* uav = nullptr;
    HRESULT hr = device->CreateUnorderedAccessView(tex, nullptr, &uav);
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(uav, nullptr);

    if (uav) uav->Release();
    tex->Release();
}

TEST_F(ViewTests, UAV_NullResourceRejected)
{
    ID3D11UnorderedAccessView* uav = nullptr;
    EXPECT_TRUE(FAILED(device->CreateUnorderedAccessView(nullptr, nullptr, &uav)));
}

TEST_F(ViewTests, UAV_NullOutputReturnsSFalse)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_UNORDERED_ACCESS);
    ASSERT_NE(tex, nullptr);

    EXPECT_EQ(device->CreateUnorderedAccessView(tex, nullptr, nullptr), S_FALSE);
    tex->Release();
}

TEST_F(ViewTests, UAV_GetDescRoundtrip)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_UNORDERED_ACCESS);
    ASSERT_NE(tex, nullptr);

    D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(tex, &desc, &uav)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC out = {};
    uav->GetDesc(&out);
    EXPECT_EQ(out.Format,             DXGI_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(out.ViewDimension,      D3D11_UAV_DIMENSION_TEXTURE2D);
    EXPECT_EQ(out.Texture2D.MipSlice, 0u);

    uav->Release();
    tex->Release();
}
