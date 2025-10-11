#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class D3D11DepthStencilViewSW final : public DeviceChildImpl<ID3D11DepthStencilView>
{
public:
    explicit D3D11DepthStencilViewSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    void STDMETHODCALLTYPE GetResource(ID3D11Resource** ppResource) override;
    void STDMETHODCALLTYPE GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc) override;
};

}
