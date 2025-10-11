#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11Texture3DSW : public DeviceChildImpl<ID3D11Texture3D1>
{
public:
    explicit Direct3D11Texture3DSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) final;

    void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension) override;
    void STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) override;
    UINT STDMETHODCALLTYPE GetEvictionPriority() override;

    void STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE3D_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_TEXTURE3D_DESC1* pDesc) override;
};

}
