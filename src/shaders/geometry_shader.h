#pragma once
#include "common/common.h"
#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11GeometryShaderSW : public DeviceChildImpl<ID3D11GeometryShader, ID3D11DeviceChild>
{
public:
    explicit Direct3D11GeometryShaderSW(ID3D11Device* device);
};

}
