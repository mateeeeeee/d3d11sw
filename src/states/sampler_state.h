#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11SamplerStateSW : public DeviceChildImpl<ID3D11SamplerState, ID3D11DeviceChild>
{
public:
    explicit Direct3D11SamplerStateSW(ID3D11Device* device);

    void STDMETHODCALLTYPE GetDesc(D3D11_SAMPLER_DESC* pDesc) override;
};

}
