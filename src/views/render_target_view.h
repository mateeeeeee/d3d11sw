#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11RenderTargetViewSW
    : public DeviceChildImpl<ID3D11RenderTargetView1,
                             ID3D11RenderTargetView,
                             ID3D11View,
                             ID3D11DeviceChild>
{
public:
    explicit Direct3D11RenderTargetViewSW(ID3D11Device* device);

    void STDMETHODCALLTYPE GetResource(ID3D11Resource** ppResource) override;
    void STDMETHODCALLTYPE GetDesc(D3D11_RENDER_TARGET_VIEW_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_RENDER_TARGET_VIEW_DESC1* pDesc) override;
};

}
