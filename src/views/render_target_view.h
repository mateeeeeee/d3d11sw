#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class D3D11RenderTargetViewSW final : public DeviceChildImpl<ID3D11RenderTargetView1>
{
public:
    explicit D3D11RenderTargetViewSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    void STDMETHODCALLTYPE GetResource(ID3D11Resource** ppResource) override;
    void STDMETHODCALLTYPE GetDesc(D3D11_RENDER_TARGET_VIEW_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_RENDER_TARGET_VIEW_DESC1* pDesc) override;
};

}
