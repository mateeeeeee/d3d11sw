#pragma once

#include "device.h"

class MarsDevice1 : public MarsDevice
{
public:
    void STDMETHODCALLTYPE GetImmediateContext1(
        ID3D11DeviceContext1** ppImmediateContext) override;

    HRESULT STDMETHODCALLTYPE CreateDeferredContext1(
        UINT ContextFlags,
        ID3D11DeviceContext1** ppDeferredContext) override;

    HRESULT STDMETHODCALLTYPE CreateBlendState1(
        const D3D11_BLEND_DESC1* pBlendStateDesc,
        ID3D11BlendState1** ppBlendState) override;

    HRESULT STDMETHODCALLTYPE CreateRasterizerState1(
        const D3D11_RASTERIZER_DESC1* pRasterizerDesc,
        ID3D11RasterizerState1** ppRasterizerState) override;

    HRESULT STDMETHODCALLTYPE CreateDeviceContextState(
        UINT Flags,
        const D3D_FEATURE_LEVEL* pFeatureLevels,
        UINT FeatureLevels,
        UINT SDKVersion,
        REFIID EmulatedInterface,
        D3D_FEATURE_LEVEL* pChosenFeatureLevel,
        ID3DDeviceContextState** ppContextState) override;

    HRESULT STDMETHODCALLTYPE OpenSharedResource1(
        HANDLE hResource,
        REFIID returnedInterface,
        void** ppResource) override;

    HRESULT STDMETHODCALLTYPE OpenSharedResourceByName(
        LPCWSTR lpName,
        DWORD dwDesiredAccess,
        REFIID returnedInterface,
        void** ppResource) override;
};
