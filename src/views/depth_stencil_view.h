#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11DepthStencilViewSW
    : public DeviceChildImpl<ID3D11DepthStencilView,
                             ID3D11View,
                             ID3D11DeviceChild>
                             {
public:
    explicit Direct3D11DepthStencilViewSW(ID3D11Device* device);

    void STDMETHODCALLTYPE GetResource(ID3D11Resource** ppResource) override;
    void STDMETHODCALLTYPE GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc) override;
};

}
