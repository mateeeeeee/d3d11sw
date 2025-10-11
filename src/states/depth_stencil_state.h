#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class D3D11DepthStencilStateSW final : public DeviceChildImpl<ID3D11DepthStencilState>
{
public:
    explicit D3D11DepthStencilStateSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    void STDMETHODCALLTYPE GetDesc(D3D11_DEPTH_STENCIL_DESC* pDesc) override;
};

}
