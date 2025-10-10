#pragma once
#include "common/common.h"
#include "common/device_child_impl.h"

class MarsHullShader : public DeviceChildImpl<ID3D11HullShader, ID3D11DeviceChild>
{
public:
    MarsHullShader(ID3D11Device* device);
};
