#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11BlendStateSW : public DeviceChildImpl<ID3D11BlendState1, ID3D11BlendState, ID3D11DeviceChild>
{
public:
    explicit Direct3D11BlendStateSW(ID3D11Device* device);

    void STDMETHODCALLTYPE GetDesc(D3D11_BLEND_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_BLEND_DESC1* pDesc) override;
};

}
