#pragma once
#include "common/common.h"
#include "common/device_child_impl.h"

class MarsVertexShader : public DeviceChildImpl<ID3D11VertexShader, ID3D11DeviceChild>
{
public:
    MarsVertexShader(ID3D11Device* device);
};
