#pragma once
#include "common/common.h"
#include "common/device_child_impl.h"

class MarsPixelShader : public DeviceChildImpl<ID3D11PixelShader, ID3D11DeviceChild>
{
public:
    MarsPixelShader(ID3D11Device* device);
};
