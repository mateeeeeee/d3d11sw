#include "device3.h"
#include "context/context.h"

HRESULT STDMETHODCALLTYPE MarsDevice3::CreateTexture2D1(
    const D3D11_TEXTURE2D_DESC1* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture2D1** ppTexture2D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice3::CreateTexture3D1(
    const D3D11_TEXTURE3D_DESC1* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture3D1** ppTexture3D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice3::CreateRasterizerState2(
    const D3D11_RASTERIZER_DESC2* pRasterizerDesc,
    ID3D11RasterizerState2** ppRasterizerState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice3::CreateShaderResourceView1(
    ID3D11Resource* pResource,
    const D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc,
    ID3D11ShaderResourceView1** ppSRView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice3::CreateUnorderedAccessView1(
    ID3D11Resource* pResource,
    const D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc,
    ID3D11UnorderedAccessView1** ppUAView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice3::CreateRenderTargetView1(
    ID3D11Resource* pResource,
    const D3D11_RENDER_TARGET_VIEW_DESC1* pDesc,
    ID3D11RenderTargetView1** ppRTView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice3::CreateQuery1(
    const D3D11_QUERY_DESC1* pQueryDesc,
    ID3D11Query1** ppQuery)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE MarsDevice3::GetImmediateContext3(
    ID3D11DeviceContext3** ppImmediateContext)
{
    if (ppImmediateContext && m_immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext3*>(m_immediateContext);
        m_immediateContext->AddRef();
    }
}

HRESULT STDMETHODCALLTYPE MarsDevice3::CreateDeferredContext3(
    UINT ContextFlags,
    ID3D11DeviceContext3** ppDeferredContext)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE MarsDevice3::WriteToSubresource(
    ID3D11Resource* pDstResource,
    UINT DstSubresource,
    const D3D11_BOX* pDstBox,
    const void* pSrcData,
    UINT SrcRowPitch,
    UINT SrcDepthPitch)
{
}

void STDMETHODCALLTYPE MarsDevice3::ReadFromSubresource(
    void* pDstData,
    UINT DstRowPitch,
    UINT DstDepthPitch,
    ID3D11Resource* pSrcResource,
    UINT SrcSubresource,
    const D3D11_BOX* pSrcBox)
{
}
