#pragma once
#include "common/common.h"
#include "common/device_child_impl.h"

class MarsGeometryShader : public DeviceChildImpl<ID3D11GeometryShader, ID3D11DeviceChild>
{
public:
    MarsGeometryShader(ID3D11Device* device);
};
