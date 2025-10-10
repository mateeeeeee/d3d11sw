#pragma once
#include "common/common.h"
#include "common/device_child_impl.h"

class MarsComputeShader : public DeviceChildImpl<ID3D11ComputeShader, ID3D11DeviceChild>
{
public:
    MarsComputeShader(ID3D11Device* device);
};
