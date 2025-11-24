#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <cstring>
#include "resources/texture2d.h"
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

TEST_F(ViewTests, ClearRTV_R8G8B8A8_WritesCorrectPixels)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(tex, nullptr, &rtv)));

    FLOAT color[4] = { 1.0f, 0.5f, 0.0f, 1.0f };
    context->ClearRenderTargetView(rtv, color);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    EXPECT_EQ(data[0], 255u);  // R = 1.0
    EXPECT_EQ(data[1], 128u);  // G ≈ 0.5
    EXPECT_EQ(data[2],   0u);  // B = 0.0
    EXPECT_EQ(data[3], 255u);  // A = 1.0

    rtv->Release();
    tex->Release();
}

TEST_F(ViewTests, ClearRTV_B8G8R8A8_WritesSwappedChannels)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(tex, nullptr, &rtv)));

    FLOAT color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };  // pure red
    context->ClearRenderTargetView(rtv, color);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    EXPECT_EQ(data[0],   0u);  // B channel
    EXPECT_EQ(data[1],   0u);  // G channel
    EXPECT_EQ(data[2], 255u);  // R channel
    EXPECT_EQ(data[3], 255u);  // A channel

    rtv->Release();
    tex->Release();
}

TEST_F(ViewTests, ClearRTV_R32Float_WritesFloat)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R32_FLOAT, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(tex, nullptr, &rtv)));

    FLOAT color[4] = { 0.75f, 0.f, 0.f, 0.f };
    context->ClearRenderTargetView(rtv, color);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    FLOAT val;
    std::memcpy(&val, data, 4);
    EXPECT_FLOAT_EQ(val, 0.75f);

    rtv->Release();
    tex->Release();
}

TEST_F(ViewTests, ClearRTV_FillsAllPixels)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET);
    ASSERT_NE(tex, nullptr);

    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(tex, nullptr, &rtv)));

    FLOAT color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    context->ClearRenderTargetView(rtv, color);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    // Check last pixel (64*64 - 1) as well
    UINT lastPixelOffset = (layout.NumRows - 1) * layout.RowPitch + (layout.RowPitch - 4);
    EXPECT_EQ(data[lastPixelOffset + 0],   0u);  
    EXPECT_EQ(data[lastPixelOffset + 1], 255u);  
    EXPECT_EQ(data[lastPixelOffset + 2],   0u);  
    EXPECT_EQ(data[lastPixelOffset + 3], 255u);  

    rtv->Release();
    tex->Release();
}

TEST_F(ViewTests, ClearDSV_D32Float_WritesDepth)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_D32_FLOAT, D3D11_BIND_DEPTH_STENCIL);
    ASSERT_NE(tex, nullptr);

    ID3D11DepthStencilView* dsv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilView(tex, nullptr, &dsv)));

    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 0.75f, 0);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    FLOAT depth;
    std::memcpy(&depth, data, 4);
    EXPECT_FLOAT_EQ(depth, 0.75f);

    dsv->Release();
    tex->Release();
}

TEST_F(ViewTests, ClearDSV_D24S8_ClearsBoth)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL);
    ASSERT_NE(tex, nullptr);

    ID3D11DepthStencilView* dsv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilView(tex, nullptr, &dsv)));

    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0xAB);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    UINT32 packed;
    std::memcpy(&packed, data, 4);
    EXPECT_EQ(packed & 0xFFu, 0xABu);                   // stencil in low byte
    EXPECT_EQ((packed >> 8) & 0xFFFFFFu, 0xFFFFFFu);    // depth = 1.0 → all 24 bits set

    dsv->Release();
    tex->Release();
}

TEST_F(ViewTests, ClearDSV_D24S8_DepthOnlyPreservesStencil)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL);
    ASSERT_NE(tex, nullptr);

    ID3D11DepthStencilView* dsv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilView(tex, nullptr, &dsv)));

    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.5f, 0x42);
    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0x00);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    UINT32 packed;
    std::memcpy(&packed, data, 4);
    EXPECT_EQ(packed & 0xFFu, 0x42u);                   // stencil preserved
    EXPECT_EQ((packed >> 8) & 0xFFFFFFu, 0xFFFFFFu);    // depth updated to 1.0

    dsv->Release();
    tex->Release();
}

TEST_F(ViewTests, ClearDSV_D16_WritesDepth)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_D16_UNORM, D3D11_BIND_DEPTH_STENCIL);
    ASSERT_NE(tex, nullptr);

    ID3D11DepthStencilView* dsv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilView(tex, nullptr, &dsv)));

    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    UINT16 d16;
    std::memcpy(&d16, data, 2);
    EXPECT_EQ(d16, 0xFFFFu);  

    dsv->Release();
    tex->Release();
}

TEST_F(ViewTests, ClearUAVUint_R32_WritesValue)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R32_UINT, D3D11_BIND_UNORDERED_ACCESS);
    ASSERT_NE(tex, nullptr);

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(tex, nullptr, &uav)));

    UINT values[4] = { 0xDEADBEEF, 0, 0, 0 };
    context->ClearUnorderedAccessViewUint(uav, values);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    UINT val;
    std::memcpy(&val, data, 4);
    EXPECT_EQ(val, 0xDEADBEEFu);

    uav->Release();
    tex->Release();
}

TEST_F(ViewTests, ClearUAVUint_R8G8B8A8_WritesChannels)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UINT, D3D11_BIND_UNORDERED_ACCESS);
    ASSERT_NE(tex, nullptr);

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(tex, nullptr, &uav)));

    UINT values[4] = { 10, 20, 30, 40 };
    context->ClearUnorderedAccessViewUint(uav, values);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    EXPECT_EQ(data[0], 10u);
    EXPECT_EQ(data[1], 20u);
    EXPECT_EQ(data[2], 30u);
    EXPECT_EQ(data[3], 40u);

    uav->Release();
    tex->Release();
}

TEST_F(ViewTests, ClearUAVFloat_R32_WritesFloat)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R32_FLOAT, D3D11_BIND_UNORDERED_ACCESS);
    ASSERT_NE(tex, nullptr);

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(tex, nullptr, &uav)));

    FLOAT values[4] = { 3.14f, 0.f, 0.f, 0.f };
    context->ClearUnorderedAccessViewFloat(uav, values);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    FLOAT val;
    std::memcpy(&val, data, 4);
    EXPECT_FLOAT_EQ(val, 3.14f);

    uav->Release();
    tex->Release();
}

TEST_F(ViewTests, ClearUAVFloat_R8G8B8A8Unorm_WritesUnorm)
{
    ID3D11Texture2D* tex = MakeTex2D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_UNORDERED_ACCESS);
    ASSERT_NE(tex, nullptr);

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(tex, nullptr, &uav)));

    FLOAT values[4] = { 1.0f, 0.0f, 0.5f, 1.0f };
    context->ClearUnorderedAccessViewFloat(uav, values);

    auto* sw     = static_cast<D3D11Texture2DSW*>(tex);
    auto  layout = sw->GetSubresourceLayout(0);
    const UINT8* data = static_cast<const UINT8*>(sw->GetDataPtr()) + layout.Offset;

    EXPECT_EQ(data[0], 255u);  // R = 1.0
    EXPECT_EQ(data[1],   0u);  // G = 0.0
    EXPECT_EQ(data[2], 128u);  // B ≈ 0.5
    EXPECT_EQ(data[3], 255u);  // A = 1.0

    uav->Release();
    tex->Release();
}
