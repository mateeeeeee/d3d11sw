#include <gtest/gtest.h>
#include <d3d11_4.h>

using namespace d3d11sw;

struct ContextStateTests : ::testing::Test
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
        ASSERT_NE(device,  nullptr);
        ASSERT_NE(context, nullptr);
    }

    void TearDown() override
    {
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }

    ID3D11Buffer* MakeBuffer(UINT byteWidth = 256, UINT bindFlags = D3D11_BIND_VERTEX_BUFFER)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = byteWidth;
        desc.Usage     = D3D11_USAGE_DEFAULT;
        desc.BindFlags = bindFlags;

        ID3D11Buffer* buf = nullptr;
        EXPECT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buf)));
        return buf;
    }

    ID3D11RasterizerState* MakeRasterizerState()
    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.FillMode        = D3D11_FILL_SOLID;
        desc.CullMode        = D3D11_CULL_BACK;
        desc.DepthClipEnable = TRUE;

        ID3D11RasterizerState* rs = nullptr;
        EXPECT_TRUE(SUCCEEDED(device->CreateRasterizerState(&desc, &rs)));
        return rs;
    }

    ID3D11BlendState* MakeBlendState()
    {
        D3D11_BLEND_DESC desc = {};
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        ID3D11BlendState* bs = nullptr;
        EXPECT_TRUE(SUCCEEDED(device->CreateBlendState(&desc, &bs)));
        return bs;
    }

    ID3D11DepthStencilState* MakeDepthStencilState()
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable    = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc      = D3D11_COMPARISON_LESS;

        ID3D11DepthStencilState* dss = nullptr;
        EXPECT_TRUE(SUCCEEDED(device->CreateDepthStencilState(&desc, &dss)));
        return dss;
    }

    ID3D11SamplerState* MakeSamplerState()
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
        EXPECT_TRUE(SUCCEEDED(device->CreateSamplerState(&desc, &ss)));
        return ss;
    }

    ID3D11Texture2D* MakeTexture2D(UINT width = 64, UINT height = 64,
                                    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                    UINT bindFlags = D3D11_BIND_RENDER_TARGET)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width      = width;
        desc.Height     = height;
        desc.MipLevels  = 1;
        desc.ArraySize  = 1;
        desc.Format     = format;
        desc.SampleDesc = { 1, 0 };
        desc.Usage      = D3D11_USAGE_DEFAULT;
        desc.BindFlags  = bindFlags;

        ID3D11Texture2D* tex = nullptr;
        EXPECT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &tex)));
        return tex;
    }

    ID3D11RenderTargetView* MakeRTV(ID3D11Texture2D* tex = nullptr)
    {
        bool ownTex = (tex == nullptr);
        if (!tex)
        {
            tex = MakeTexture2D();
        }

        ID3D11RenderTargetView* rtv = nullptr;
        EXPECT_TRUE(SUCCEEDED(device->CreateRenderTargetView(tex, nullptr, &rtv)));
        if (ownTex) { tex->Release(); }
        return rtv;
    }

    ID3D11UnorderedAccessView* MakeUAV()
    {
        ID3D11Buffer* buf = MakeBuffer(256, D3D11_BIND_UNORDERED_ACCESS);
        D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
        desc.Format             = DXGI_FORMAT_R32_UINT;
        desc.ViewDimension      = D3D11_UAV_DIMENSION_BUFFER;
        desc.Buffer.NumElements = 64;

        ID3D11UnorderedAccessView* uav = nullptr;
        EXPECT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &desc, &uav)));
        buf->Release();
        return uav;
    }
};

TEST_F(ContextStateTests, IA_InputLayout_RoundTrip)
{
    D3D11_INPUT_ELEMENT_DESC elem = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    ID3D11InputLayout* layout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(&elem, 1, nullptr, 0, &layout)));

    context->IASetInputLayout(layout);

    ID3D11InputLayout* out = nullptr;
    context->IAGetInputLayout(&out);
    EXPECT_EQ(out, layout);
    if (out) out->Release();

    layout->Release();
}

TEST_F(ContextStateTests, IA_InputLayout_RefCounting)
{
    D3D11_INPUT_ELEMENT_DESC elem = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    ID3D11InputLayout* layout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(&elem, 1, nullptr, 0, &layout)));

    context->IASetInputLayout(layout);   // context holds a ref
    ID3D11InputLayout* out = nullptr;
    context->IAGetInputLayout(&out);     // Get AddRefs again
    ASSERT_NE(out, nullptr);
    out->Release();                      // release the Get ref

    context->IASetInputLayout(nullptr);  // context releases its ref
    layout->Release();                   // back to 0
}

TEST_F(ContextStateTests, IA_VertexBuffers_RoundTrip)
{
    ID3D11Buffer* vb = MakeBuffer(64, D3D11_BIND_VERTEX_BUFFER);
    ASSERT_NE(vb, nullptr);

    UINT stride = 16, offset = 8;
    context->IASetVertexBuffers(2, 1, &vb, &stride, &offset);

    ID3D11Buffer* outBuf  = nullptr;
    UINT          outStr  = 0;
    UINT          outOff  = 0;
    context->IAGetVertexBuffers(2, 1, &outBuf, &outStr, &outOff);

    EXPECT_EQ(outBuf, vb);
    EXPECT_EQ(outStr, 16u);
    EXPECT_EQ(outOff, 8u);
    if (outBuf) outBuf->Release();

    vb->Release();
}

TEST_F(ContextStateTests, IA_VertexBuffers_NullUnbinds)
{
    ID3D11Buffer* vb = MakeBuffer();
    ASSERT_NE(vb, nullptr);
    UINT stride = 16, offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

    context->IASetVertexBuffers(0, 1, nullptr, nullptr, nullptr);

    ID3D11Buffer* out = nullptr;
    context->IAGetVertexBuffers(0, 1, &out, nullptr, nullptr);
    EXPECT_EQ(out, nullptr);

    vb->Release();
}

TEST_F(ContextStateTests, IA_IndexBuffer_RoundTrip)
{
    ID3D11Buffer* ib = MakeBuffer(64, D3D11_BIND_INDEX_BUFFER);
    ASSERT_NE(ib, nullptr);

    context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 4);

    ID3D11Buffer* outBuf    = nullptr;
    DXGI_FORMAT   outFormat = DXGI_FORMAT_UNKNOWN;
    UINT          outOffset = 0;
    context->IAGetIndexBuffer(&outBuf, &outFormat, &outOffset);

    EXPECT_EQ(outBuf,    ib);
    EXPECT_EQ(outFormat, DXGI_FORMAT_R32_UINT);
    EXPECT_EQ(outOffset, 4u);
    if (outBuf) outBuf->Release();

    ib->Release();
}

TEST_F(ContextStateTests, IA_PrimitiveTopology_RoundTrip)
{
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_PRIMITIVE_TOPOLOGY out = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    context->IAGetPrimitiveTopology(&out);
    EXPECT_EQ(out, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

TEST_F(ContextStateTests, VS_ConstantBuffers_RoundTrip)
{
    ID3D11Buffer* cb0 = MakeBuffer(256, D3D11_BIND_CONSTANT_BUFFER);
    ID3D11Buffer* cb1 = MakeBuffer(256, D3D11_BIND_CONSTANT_BUFFER);
    ASSERT_NE(cb0, nullptr);
    ASSERT_NE(cb1, nullptr);

    ID3D11Buffer* cbs[2] = { cb0, cb1 };
    context->VSSetConstantBuffers(3, 2, cbs);

    ID3D11Buffer* out[2] = {};
    context->VSGetConstantBuffers(3, 2, out);

    EXPECT_EQ(out[0], cb0);
    EXPECT_EQ(out[1], cb1);
    if (out[0]) out[0]->Release();
    if (out[1]) out[1]->Release();

    cb0->Release();
    cb1->Release();
}

TEST_F(ContextStateTests, PS_ConstantBuffers_RoundTrip)
{
    ID3D11Buffer* cb = MakeBuffer(256, D3D11_BIND_CONSTANT_BUFFER);
    ASSERT_NE(cb, nullptr);

    context->PSSetConstantBuffers(0, 1, &cb);

    ID3D11Buffer* out = nullptr;
    context->PSGetConstantBuffers(0, 1, &out);
    EXPECT_EQ(out, cb);
    if (out) out->Release();

    cb->Release();
}

TEST_F(ContextStateTests, VS_Samplers_RoundTrip)
{
    ID3D11SamplerState* ss = MakeSamplerState();
    ASSERT_NE(ss, nullptr);

    context->VSSetSamplers(1, 1, &ss);

    ID3D11SamplerState* out = nullptr;
    context->VSGetSamplers(1, 1, &out);
    EXPECT_EQ(out, ss);
    if (out) out->Release();

    ss->Release();
}

TEST_F(ContextStateTests, PS_Samplers_RoundTrip)
{
    ID3D11SamplerState* ss = MakeSamplerState();
    ASSERT_NE(ss, nullptr);

    context->PSSetSamplers(0, 1, &ss);

    ID3D11SamplerState* out = nullptr;
    context->PSGetSamplers(0, 1, &out);
    EXPECT_EQ(out, ss);
    if (out) out->Release();

    ss->Release();
}

TEST_F(ContextStateTests, RS_State_RoundTrip)
{
    ID3D11RasterizerState* rs = MakeRasterizerState();
    ASSERT_NE(rs, nullptr);

    context->RSSetState(rs);

    ID3D11RasterizerState* out = nullptr;
    context->RSGetState(&out);
    EXPECT_EQ(out, rs);
    if (out) out->Release();

    rs->Release();
}

TEST_F(ContextStateTests, RS_Viewports_RoundTrip)
{
    D3D11_VIEWPORT vp = { 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f };
    context->RSSetViewports(1, &vp);

    UINT count = 0;
    context->RSGetViewports(&count, nullptr);
    EXPECT_EQ(count, 1u);

    D3D11_VIEWPORT out = {};
    context->RSGetViewports(&count, &out);
    EXPECT_FLOAT_EQ(out.Width,  1280.0f);
    EXPECT_FLOAT_EQ(out.Height, 720.0f);
}

TEST_F(ContextStateTests, RS_ScissorRects_RoundTrip)
{
    D3D11_RECT rect = { 10, 20, 300, 400 };
    context->RSSetScissorRects(1, &rect);

    UINT count = 0;
    context->RSGetScissorRects(&count, nullptr);
    EXPECT_EQ(count, 1u);

    D3D11_RECT out = {};
    context->RSGetScissorRects(&count, &out);
    EXPECT_EQ(out.left,   10);
    EXPECT_EQ(out.top,    20);
    EXPECT_EQ(out.right,  300);
    EXPECT_EQ(out.bottom, 400);
}

TEST_F(ContextStateTests, OM_BlendState_RoundTrip)
{
    ID3D11BlendState* bs = MakeBlendState();
    ASSERT_NE(bs, nullptr);

    FLOAT factor[4] = { 0.1f, 0.2f, 0.3f, 0.4f };
    context->OMSetBlendState(bs, factor, 0xABCDEF01);

    ID3D11BlendState* outBs     = nullptr;
    FLOAT             outFactor[4] = {};
    UINT              outMask   = 0;
    context->OMGetBlendState(&outBs, outFactor, &outMask);

    EXPECT_EQ(outBs, bs);
    EXPECT_FLOAT_EQ(outFactor[0], 0.1f);
    EXPECT_FLOAT_EQ(outFactor[1], 0.2f);
    EXPECT_FLOAT_EQ(outFactor[2], 0.3f);
    EXPECT_FLOAT_EQ(outFactor[3], 0.4f);
    EXPECT_EQ(outMask, 0xABCDEF01u);
    if (outBs) outBs->Release();

    bs->Release();
}

TEST_F(ContextStateTests, OM_BlendState_NullFactorDefaultsToOne)
{
    ID3D11BlendState* bs = MakeBlendState();
    ASSERT_NE(bs, nullptr);

    context->OMSetBlendState(bs, nullptr, 0xFFFFFFFF);

    FLOAT outFactor[4] = {};
    context->OMGetBlendState(nullptr, outFactor, nullptr);
    EXPECT_FLOAT_EQ(outFactor[0], 1.0f);
    EXPECT_FLOAT_EQ(outFactor[1], 1.0f);
    EXPECT_FLOAT_EQ(outFactor[2], 1.0f);
    EXPECT_FLOAT_EQ(outFactor[3], 1.0f);

    bs->Release();
}

TEST_F(ContextStateTests, OM_DepthStencilState_RoundTrip)
{
    ID3D11DepthStencilState* dss = MakeDepthStencilState();
    ASSERT_NE(dss, nullptr);

    context->OMSetDepthStencilState(dss, 42);

    ID3D11DepthStencilState* outDss = nullptr;
    UINT                     outRef = 0;
    context->OMGetDepthStencilState(&outDss, &outRef);

    EXPECT_EQ(outDss, dss);
    EXPECT_EQ(outRef, 42u);
    if (outDss) outDss->Release();

    dss->Release();
}

TEST_F(ContextStateTests, ClearState_NullsBindings)
{
    ID3D11Buffer*          vb  = MakeBuffer(64, D3D11_BIND_VERTEX_BUFFER);
    ID3D11Buffer*          cb  = MakeBuffer(256, D3D11_BIND_CONSTANT_BUFFER);
    ID3D11RasterizerState* rs  = MakeRasterizerState();
    ID3D11BlendState*      bs  = MakeBlendState();
    ASSERT_NE(vb, nullptr);
    ASSERT_NE(cb, nullptr);
    ASSERT_NE(rs, nullptr);
    ASSERT_NE(bs, nullptr);

    UINT stride = 16, offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->VSSetConstantBuffers(0, 1, &cb);
    context->RSSetState(rs);
    FLOAT factor[4] = { 1, 1, 1, 1 };
    context->OMSetBlendState(bs, factor, 0xFFFFFFFF);

    context->ClearState();

    ID3D11Buffer*          outVb = nullptr;
    ID3D11Buffer*          outCb = nullptr;
    ID3D11RasterizerState* outRs = nullptr;
    ID3D11BlendState*      outBs = nullptr;

    context->IAGetVertexBuffers(0, 1, &outVb, nullptr, nullptr);
    context->VSGetConstantBuffers(0, 1, &outCb);
    context->RSGetState(&outRs);
    context->OMGetBlendState(&outBs, nullptr, nullptr);

    EXPECT_EQ(outVb, nullptr);
    EXPECT_EQ(outCb, nullptr);
    EXPECT_EQ(outRs, nullptr);
    EXPECT_EQ(outBs, nullptr);

    vb->Release();
    cb->Release();
    rs->Release();
    bs->Release();
}

TEST_F(ContextStateTests, ClearState_ResetsBlendDefaults)
{
    FLOAT factor[4] = { 0, 0, 0, 0 };
    context->OMSetBlendState(nullptr, factor, 0x00000000);

    context->ClearState();

    FLOAT outFactor[4] = {};
    UINT  outMask      = 0;
    context->OMGetBlendState(nullptr, outFactor, &outMask);

    EXPECT_FLOAT_EQ(outFactor[0], 1.0f);
    EXPECT_FLOAT_EQ(outFactor[1], 1.0f);
    EXPECT_FLOAT_EQ(outFactor[2], 1.0f);
    EXPECT_FLOAT_EQ(outFactor[3], 1.0f);
    EXPECT_EQ(outMask, 0xFFFFFFFFu);
}

TEST_F(ContextStateTests, ClearState_ReleasesRefs)
{
    ID3D11RasterizerState* rs = MakeRasterizerState();
    ASSERT_NE(rs, nullptr);

    context->RSSetState(rs);
    context->ClearState();

    rs->Release();
}

TEST_F(ContextStateTests, OM_SetRTVsAndUAVs_BothBindings)
{
    ID3D11RenderTargetView* rtv = MakeRTV();
    ID3D11UnorderedAccessView* uav = MakeUAV();
    ASSERT_NE(rtv, nullptr);
    ASSERT_NE(uav, nullptr);

    context->OMSetRenderTargetsAndUnorderedAccessViews(
        1, &rtv, nullptr,
        1, 1, &uav, nullptr);

    ID3D11RenderTargetView* outRTV = nullptr;
    context->OMGetRenderTargets(1, &outRTV, nullptr);
    EXPECT_EQ(outRTV, rtv);
    if (outRTV) { outRTV->Release(); }

    ID3D11UnorderedAccessView* outUAV = nullptr;
    context->OMGetRenderTargetsAndUnorderedAccessViews(
        0, nullptr, nullptr,
        1, 1, &outUAV);
    EXPECT_EQ(outUAV, uav);
    if (outUAV) { outUAV->Release(); }

    rtv->Release();
    uav->Release();
}

TEST_F(ContextStateTests, OM_SetRTVsAndUAVs_KeepRTVs)
{
    ID3D11RenderTargetView* rtv = MakeRTV();
    ASSERT_NE(rtv, nullptr);

    context->OMSetRenderTargets(1, &rtv, nullptr);

    ID3D11UnorderedAccessView* uav = MakeUAV();
    ASSERT_NE(uav, nullptr);

    context->OMSetRenderTargetsAndUnorderedAccessViews(
        D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
        0, 1, &uav, nullptr);

    ID3D11RenderTargetView* outRTV = nullptr;
    context->OMGetRenderTargets(1, &outRTV, nullptr);
    EXPECT_EQ(outRTV, rtv);
    if (outRTV) { outRTV->Release(); }

    rtv->Release();
    uav->Release();
}

TEST_F(ContextStateTests, OM_SetRTVsAndUAVs_KeepUAVs)
{
    ID3D11UnorderedAccessView* uav = MakeUAV();
    ASSERT_NE(uav, nullptr);

    context->OMSetRenderTargetsAndUnorderedAccessViews(
        0, nullptr, nullptr,
        0, 1, &uav, nullptr);

    ID3D11RenderTargetView* rtv = MakeRTV();
    ASSERT_NE(rtv, nullptr);

    context->OMSetRenderTargetsAndUnorderedAccessViews(
        1, &rtv, nullptr,
        D3D11_KEEP_UNORDERED_ACCESS_VIEWS, 0, nullptr, nullptr);

    ID3D11UnorderedAccessView* outUAV = nullptr;
    context->OMGetRenderTargetsAndUnorderedAccessViews(
        0, nullptr, nullptr,
        0, 1, &outUAV);
    EXPECT_EQ(outUAV, uav);
    if (outUAV) { outUAV->Release(); }

    rtv->Release();
    uav->Release();
}

TEST_F(ContextStateTests, CopyStructureCount_WritesZero)
{
    ID3D11Buffer* dst = MakeBuffer(256, D3D11_BIND_UNORDERED_ACCESS);
    ASSERT_NE(dst, nullptr);

    UINT initData = 0xDEADBEEF;
    context->UpdateSubresource(dst, 0, nullptr, &initData, 0, 0);

    ID3D11UnorderedAccessView* uav = MakeUAV();
    ASSERT_NE(uav, nullptr);

    context->CopyStructureCount(dst, 0, uav);

    D3D11_BUFFER_DESC stagingDesc = {};
    stagingDesc.ByteWidth      = 256;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, dst);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));
    UINT result = 0;
    std::memcpy(&result, mapped.pData, sizeof(UINT));
    context->Unmap(staging, 0);

    EXPECT_EQ(result, 0u);

    staging->Release();
    uav->Release();
    dst->Release();
}
