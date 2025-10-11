#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class D3D11Texture2DSW final : public DeviceChildImpl<ID3D11Texture2D1>
{
public:
    explicit D3D11Texture2DSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension) override;
    void STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) override;
    UINT STDMETHODCALLTYPE GetEvictionPriority() override;

    void STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE2D_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_TEXTURE2D_DESC1* pDesc) override;
};

}
