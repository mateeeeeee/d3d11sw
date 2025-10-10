#pragma once

#include "common/device_child_impl.h"

class MarsDepthStencilState : public DeviceChildImpl<ID3D11DepthStencilState, ID3D11DeviceChild>
{
public:
    MarsDepthStencilState(ID3D11Device* device);

    void STDMETHODCALLTYPE GetDesc(D3D11_DEPTH_STENCIL_DESC* pDesc) override;
};
