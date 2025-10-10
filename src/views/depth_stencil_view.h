#pragma once

#include "common/device_child_impl.h"

class MarsDepthStencilView
    : public DeviceChildImpl<ID3D11DepthStencilView,
                             ID3D11View,
                                                          ID3D11DeviceChild>
                             {
public:
    MarsDepthStencilView(ID3D11Device* device);

    void STDMETHODCALLTYPE GetResource(ID3D11Resource** ppResource) override;

    void STDMETHODCALLTYPE GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc) override;
};
