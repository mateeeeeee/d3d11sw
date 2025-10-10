#pragma once

#include "common/device_child_impl.h"

class MarsSamplerState : public DeviceChildImpl<ID3D11SamplerState, ID3D11DeviceChild>
{
public:
    MarsSamplerState(ID3D11Device* device);

    void STDMETHODCALLTYPE GetDesc(D3D11_SAMPLER_DESC* pDesc) override;
};
