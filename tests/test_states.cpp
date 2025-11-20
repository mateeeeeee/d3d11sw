#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "misc/input_layout.h"

using namespace d3d11sw;

struct StateTests : ::testing::Test
{
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* context = nullptr;

    void SetUp() override
    {
        D3D_FEATURE_LEVEL featureLevel;
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
    }

    void TearDown() override
    {
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }
};

// ---- SamplerState ----------------------------------------------------------

TEST_F(StateTests, SamplerState_Creation)
{
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.MaxAnisotropy  = 1;
    desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    desc.MaxLOD         = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* ss = nullptr;
    HRESULT hr = device->CreateSamplerState(&desc, &ss);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(ss, nullptr);
    ss->Release();
}

TEST_F(StateTests, SamplerState_GetDescRoundtrip)
{
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    desc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressV       = D3D11_TEXTURE_ADDRESS_MIRROR;
    desc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
    desc.MipLODBias     = 1.5f;
    desc.MaxAnisotropy  = 4;
    desc.ComparisonFunc = D3D11_COMPARISON_LESS;
    desc.MaxLOD         = 8.0f;

    ID3D11SamplerState* ss = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateSamplerState(&desc, &ss)));

    D3D11_SAMPLER_DESC out = {};
    ss->GetDesc(&out);
    EXPECT_EQ(out.Filter,         D3D11_FILTER_MIN_MAG_MIP_POINT);
    EXPECT_EQ(out.AddressU,       D3D11_TEXTURE_ADDRESS_CLAMP);
    EXPECT_EQ(out.AddressV,       D3D11_TEXTURE_ADDRESS_MIRROR);
    EXPECT_EQ(out.AddressW,       D3D11_TEXTURE_ADDRESS_BORDER);
    EXPECT_FLOAT_EQ(out.MipLODBias, 1.5f);
    EXPECT_EQ(out.MaxAnisotropy,  4u);
    EXPECT_EQ(out.ComparisonFunc, D3D11_COMPARISON_LESS);
    EXPECT_FLOAT_EQ(out.MaxLOD,   8.0f);

    ss->Release();
}

TEST_F(StateTests, SamplerState_NullDescRejected)
{
    ID3D11SamplerState* ss = nullptr;
    EXPECT_TRUE(FAILED(device->CreateSamplerState(nullptr, &ss)));
}

TEST_F(StateTests, SamplerState_NullOutputReturnsSFalse)
{
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.MaxAnisotropy  = 1;
    desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    desc.MaxLOD         = D3D11_FLOAT32_MAX;

    EXPECT_EQ(device->CreateSamplerState(&desc, nullptr), S_FALSE);
}

// ---- DepthStencilState -----------------------------------------------------

TEST_F(StateTests, DepthStencilState_Creation)
{
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable    = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc      = D3D11_COMPARISON_LESS;
    desc.StencilEnable  = FALSE;
    desc.StencilReadMask  = D3D11_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

    ID3D11DepthStencilState* dss = nullptr;
    HRESULT hr = device->CreateDepthStencilState(&desc, &dss);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(dss, nullptr);
    dss->Release();
}

TEST_F(StateTests, DepthStencilState_GetDescRoundtrip)
{
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable    = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc      = D3D11_COMPARISON_GREATER_EQUAL;
    desc.StencilEnable  = TRUE;
    desc.StencilReadMask  = 0xAA;
    desc.StencilWriteMask = 0x55;

    ID3D11DepthStencilState* dss = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilState(&desc, &dss)));

    D3D11_DEPTH_STENCIL_DESC out = {};
    dss->GetDesc(&out);
    EXPECT_EQ(out.DepthEnable,      TRUE);
    EXPECT_EQ(out.DepthWriteMask,   D3D11_DEPTH_WRITE_MASK_ZERO);
    EXPECT_EQ(out.DepthFunc,        D3D11_COMPARISON_GREATER_EQUAL);
    EXPECT_EQ(out.StencilEnable,    TRUE);
    EXPECT_EQ(out.StencilReadMask,  0xAAu);
    EXPECT_EQ(out.StencilWriteMask, 0x55u);

    dss->Release();
}

TEST_F(StateTests, DepthStencilState_NullDescRejected)
{
    ID3D11DepthStencilState* dss = nullptr;
    EXPECT_TRUE(FAILED(device->CreateDepthStencilState(nullptr, &dss)));
}

TEST_F(StateTests, DepthStencilState_NullOutputReturnsSFalse)
{
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable    = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc      = D3D11_COMPARISON_LESS;

    EXPECT_EQ(device->CreateDepthStencilState(&desc, nullptr), S_FALSE);
}

// ---- BlendState ------------------------------------------------------------

TEST_F(StateTests, BlendState_Creation)
{
    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].BlendEnable           = FALSE;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ID3D11BlendState* bs = nullptr;
    HRESULT hr = device->CreateBlendState(&desc, &bs);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(bs, nullptr);
    bs->Release();
}

TEST_F(StateTests, BlendState_GetDescRoundtrip)
{
    D3D11_BLEND_DESC desc = {};
    desc.AlphaToCoverageEnable  = FALSE;
    desc.IndependentBlendEnable = TRUE;
    desc.RenderTarget[0].BlendEnable           = TRUE;
    desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ID3D11BlendState* bs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBlendState(&desc, &bs)));

    D3D11_BLEND_DESC out = {};
    bs->GetDesc(&out);
    EXPECT_EQ(out.IndependentBlendEnable,                    TRUE);
    EXPECT_EQ(out.RenderTarget[0].BlendEnable,               TRUE);
    EXPECT_EQ(out.RenderTarget[0].SrcBlend,                  D3D11_BLEND_SRC_ALPHA);
    EXPECT_EQ(out.RenderTarget[0].DestBlend,                 D3D11_BLEND_INV_SRC_ALPHA);
    EXPECT_EQ(out.RenderTarget[0].BlendOp,                   D3D11_BLEND_OP_ADD);
    EXPECT_EQ(out.RenderTarget[0].RenderTargetWriteMask,     D3D11_COLOR_WRITE_ENABLE_ALL);

    bs->Release();
}

TEST_F(StateTests, BlendState1_GetDesc1Roundtrip)
{
    ID3D11Device1* device1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(&device1)));

    D3D11_BLEND_DESC1 desc = {};
    desc.RenderTarget[0].BlendEnable    = FALSE;
    desc.RenderTarget[0].LogicOpEnable  = TRUE;
    desc.RenderTarget[0].LogicOp        = D3D11_LOGIC_OP_COPY;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ID3D11BlendState1* bs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device1->CreateBlendState1(&desc, &bs)));

    D3D11_BLEND_DESC1 out = {};
    bs->GetDesc1(&out);
    EXPECT_EQ(out.RenderTarget[0].LogicOpEnable, TRUE);
    EXPECT_EQ(out.RenderTarget[0].LogicOp,       D3D11_LOGIC_OP_COPY);

    bs->Release();
    device1->Release();
}

TEST_F(StateTests, BlendState_NullDescRejected)
{
    ID3D11BlendState* bs = nullptr;
    EXPECT_TRUE(FAILED(device->CreateBlendState(nullptr, &bs)));
}

TEST_F(StateTests, BlendState_NullOutputReturnsSFalse)
{
    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    EXPECT_EQ(device->CreateBlendState(&desc, nullptr), S_FALSE);
}

// ---- RasterizerState -------------------------------------------------------

TEST_F(StateTests, RasterizerState_Creation)
{
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode        = D3D11_FILL_SOLID;
    desc.CullMode        = D3D11_CULL_BACK;
    desc.DepthClipEnable = TRUE;

    ID3D11RasterizerState* rs = nullptr;
    HRESULT hr = device->CreateRasterizerState(&desc, &rs);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(rs, nullptr);
    rs->Release();
}

TEST_F(StateTests, RasterizerState_GetDescRoundtrip)
{
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode              = D3D11_FILL_WIREFRAME;
    desc.CullMode              = D3D11_CULL_FRONT;
    desc.FrontCounterClockwise = TRUE;
    desc.DepthBias             = 100;
    desc.DepthBiasClamp        = 0.5f;
    desc.SlopeScaledDepthBias  = 2.0f;
    desc.DepthClipEnable       = TRUE;
    desc.ScissorEnable         = TRUE;
    desc.MultisampleEnable     = FALSE;
    desc.AntialiasedLineEnable = TRUE;

    ID3D11RasterizerState* rs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRasterizerState(&desc, &rs)));

    D3D11_RASTERIZER_DESC out = {};
    rs->GetDesc(&out);
    EXPECT_EQ(out.FillMode,              D3D11_FILL_WIREFRAME);
    EXPECT_EQ(out.CullMode,              D3D11_CULL_FRONT);
    EXPECT_EQ(out.FrontCounterClockwise, TRUE);
    EXPECT_EQ(out.DepthBias,             100);
    EXPECT_FLOAT_EQ(out.DepthBiasClamp,       0.5f);
    EXPECT_FLOAT_EQ(out.SlopeScaledDepthBias, 2.0f);
    EXPECT_EQ(out.DepthClipEnable,       TRUE);
    EXPECT_EQ(out.ScissorEnable,         TRUE);
    EXPECT_EQ(out.AntialiasedLineEnable, TRUE);

    rs->Release();
}

TEST_F(StateTests, RasterizerState2_GetDesc2Roundtrip)
{
    ID3D11Device3* device3 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(&device3)));

    D3D11_RASTERIZER_DESC2 desc = {};
    desc.FillMode              = D3D11_FILL_SOLID;
    desc.CullMode              = D3D11_CULL_BACK;
    desc.DepthClipEnable       = TRUE;
    desc.ForcedSampleCount     = 4;
    desc.ConservativeRaster    = D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON;

    ID3D11RasterizerState2* rs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device3->CreateRasterizerState2(&desc, &rs)));

    D3D11_RASTERIZER_DESC2 out = {};
    rs->GetDesc2(&out);
    EXPECT_EQ(out.ForcedSampleCount,  4u);
    EXPECT_EQ(out.ConservativeRaster, D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON);

    rs->Release();
    device3->Release();
}

TEST_F(StateTests, RasterizerState_GetDesc1PreservesBaseFields)
{
    ID3D11Device1* device1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->QueryInterface(&device1)));

    D3D11_RASTERIZER_DESC1 desc = {};
    desc.FillMode          = D3D11_FILL_WIREFRAME;
    desc.CullMode          = D3D11_CULL_NONE;
    desc.DepthClipEnable   = TRUE;
    desc.ForcedSampleCount = 2;

    ID3D11RasterizerState1* rs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device1->CreateRasterizerState1(&desc, &rs)));

    D3D11_RASTERIZER_DESC1 out = {};
    rs->GetDesc1(&out);
    EXPECT_EQ(out.FillMode,          D3D11_FILL_WIREFRAME);
    EXPECT_EQ(out.CullMode,          D3D11_CULL_NONE);
    EXPECT_EQ(out.ForcedSampleCount, 2u);

    rs->Release();
    device1->Release();
}

TEST_F(StateTests, RasterizerState_NullDescRejected)
{
    ID3D11RasterizerState* rs = nullptr;
    EXPECT_TRUE(FAILED(device->CreateRasterizerState(nullptr, &rs)));
}

TEST_F(StateTests, RasterizerState_NullOutputReturnsSFalse)
{
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode        = D3D11_FILL_SOLID;
    desc.CullMode        = D3D11_CULL_BACK;
    desc.DepthClipEnable = TRUE;

    EXPECT_EQ(device->CreateRasterizerState(&desc, nullptr), S_FALSE);
}

// ---- InputLayout -----------------------------------------------------------

TEST_F(StateTests, InputLayout_Creation)
{
    D3D11_INPUT_ELEMENT_DESC elems[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ID3D11InputLayout* layout = nullptr;
    HRESULT hr = device->CreateInputLayout(elems, 2, nullptr, 0, &layout);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(layout, nullptr);
    layout->Release();
}

TEST_F(StateTests, InputLayout_ElementsStoredCorrectly)
{
    D3D11_INPUT_ELEMENT_DESC elems[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ID3D11InputLayout* layout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(elems, 3, nullptr, 0, &layout)));

    auto* sw = static_cast<D3D11InputLayoutSW*>(layout);
    ASSERT_EQ(sw->GetElements().size(), 3u);
    EXPECT_STREQ(sw->GetElements()[0].SemanticName, "POSITION");
    EXPECT_STREQ(sw->GetElements()[1].SemanticName, "NORMAL");
    EXPECT_STREQ(sw->GetElements()[2].SemanticName, "TEXCOORD");
    EXPECT_EQ(sw->GetElements()[2].AlignedByteOffset, 24u);

    layout->Release();
}

TEST_F(StateTests, InputLayout_SemanticNameOwnership)
{
    // SemanticName pointers must remain valid after the source array is gone
    ID3D11InputLayout* layout = nullptr;
    {
        D3D11_INPUT_ELEMENT_DESC elem = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
        ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(&elem, 1, nullptr, 0, &layout)));
    }

    auto* sw = static_cast<D3D11InputLayoutSW*>(layout);
    EXPECT_STREQ(sw->GetElements()[0].SemanticName, "POSITION");

    layout->Release();
}

TEST_F(StateTests, InputLayout_NullElementsRejected)
{
    ID3D11InputLayout* layout = nullptr;
    EXPECT_TRUE(FAILED(device->CreateInputLayout(nullptr, 1, nullptr, 0, &layout)));
}

TEST_F(StateTests, InputLayout_ZeroElementsRejected)
{
    D3D11_INPUT_ELEMENT_DESC elem = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    ID3D11InputLayout* layout = nullptr;
    EXPECT_TRUE(FAILED(device->CreateInputLayout(&elem, 0, nullptr, 0, &layout)));
}

TEST_F(StateTests, InputLayout_NullOutputReturnsSFalse)
{
    D3D11_INPUT_ELEMENT_DESC elem = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    EXPECT_EQ(device->CreateInputLayout(&elem, 1, nullptr, 0, nullptr), S_FALSE);
}
