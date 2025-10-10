#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11DepthStencilStateSW : public DeviceChildImpl<ID3D11DepthStencilState, ID3D11DeviceChild>
{
public:
    explicit Direct3D11DepthStencilStateSW(ID3D11Device* device);

    void STDMETHODCALLTYPE GetDesc(D3D11_DEPTH_STENCIL_DESC* pDesc) override;
};

}
