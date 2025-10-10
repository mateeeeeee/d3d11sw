#pragma once

#include "common/device_child_impl.h"

class MarsTexture2D : public DeviceChildImpl<ID3D11Texture2D1, ID3D11Texture2D, ID3D11Resource, ID3D11DeviceChild>
{
public:
    MarsTexture2D(ID3D11Device* device);

    void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension) override;
    void STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) override;
    UINT STDMETHODCALLTYPE GetEvictionPriority() override;

    void STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE2D_DESC* pDesc) override;

    void STDMETHODCALLTYPE GetDesc1(D3D11_TEXTURE2D_DESC1* pDesc) override;
};
