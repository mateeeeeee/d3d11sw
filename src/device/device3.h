#pragma once

#include "device2.h"

class MarsDevice3 : public MarsDevice2
{
public:
    HRESULT STDMETHODCALLTYPE CreateTexture2D1(
        const D3D11_TEXTURE2D_DESC1* pDesc,
        const D3D11_SUBRESOURCE_DATA* pInitialData,
        ID3D11Texture2D1** ppTexture2D) override;

    HRESULT STDMETHODCALLTYPE CreateTexture3D1(
        const D3D11_TEXTURE3D_DESC1* pDesc,
        const D3D11_SUBRESOURCE_DATA* pInitialData,
        ID3D11Texture3D1** ppTexture3D) override;

    HRESULT STDMETHODCALLTYPE CreateRasterizerState2(
        const D3D11_RASTERIZER_DESC2* pRasterizerDesc,
        ID3D11RasterizerState2** ppRasterizerState) override;

    HRESULT STDMETHODCALLTYPE CreateShaderResourceView1(
        ID3D11Resource* pResource,
        const D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc,
        ID3D11ShaderResourceView1** ppSRView) override;

    HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView1(
        ID3D11Resource* pResource,
        const D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc,
        ID3D11UnorderedAccessView1** ppUAView) override;

    HRESULT STDMETHODCALLTYPE CreateRenderTargetView1(
        ID3D11Resource* pResource,
        const D3D11_RENDER_TARGET_VIEW_DESC1* pDesc,
        ID3D11RenderTargetView1** ppRTView) override;

    HRESULT STDMETHODCALLTYPE CreateQuery1(
        const D3D11_QUERY_DESC1* pQueryDesc,
        ID3D11Query1** ppQuery) override;

    void STDMETHODCALLTYPE GetImmediateContext3(
        ID3D11DeviceContext3** ppImmediateContext) override;

    HRESULT STDMETHODCALLTYPE CreateDeferredContext3(
        UINT ContextFlags,
        ID3D11DeviceContext3** ppDeferredContext) override;

    void STDMETHODCALLTYPE WriteToSubresource(
        ID3D11Resource* pDstResource,
        UINT DstSubresource,
        const D3D11_BOX* pDstBox,
        const void* pSrcData,
        UINT SrcRowPitch,
        UINT SrcDepthPitch) override;

    void STDMETHODCALLTYPE ReadFromSubresource(
        void* pDstData,
        UINT DstRowPitch,
        UINT DstDepthPitch,
        ID3D11Resource* pSrcResource,
        UINT SrcSubresource,
        const D3D11_BOX* pSrcBox) override;
};
