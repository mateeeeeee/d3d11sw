#include "device1.h"
#include "context/context.h"

void STDMETHODCALLTYPE MarsDevice1::GetImmediateContext1(
    ID3D11DeviceContext1** ppImmediateContext)
{
    if (ppImmediateContext && m_immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext1*>(m_immediateContext);
        m_immediateContext->AddRef();
    }
}

HRESULT STDMETHODCALLTYPE MarsDevice1::CreateDeferredContext1(
    UINT ContextFlags,
    ID3D11DeviceContext1** ppDeferredContext)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice1::CreateBlendState1(
    const D3D11_BLEND_DESC1* pBlendStateDesc,
    ID3D11BlendState1** ppBlendState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice1::CreateRasterizerState1(
    const D3D11_RASTERIZER_DESC1* pRasterizerDesc,
    ID3D11RasterizerState1** ppRasterizerState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice1::CreateDeviceContextState(
    UINT Flags,
    const D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    REFIID EmulatedInterface,
    D3D_FEATURE_LEVEL* pChosenFeatureLevel,
    ID3DDeviceContextState** ppContextState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice1::OpenSharedResource1(
    HANDLE hResource,
    REFIID returnedInterface,
    void** ppResource)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice1::OpenSharedResourceByName(
    LPCWSTR lpName,
    DWORD dwDesiredAccess,
    REFIID returnedInterface,
    void** ppResource)
{
    return E_NOTIMPL;
}
