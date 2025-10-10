#pragma once
#include "common/common.h"
#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11VertexShaderSW : public DeviceChildImpl<ID3D11VertexShader, ID3D11DeviceChild>
{
public:
    explicit Direct3D11VertexShaderSW(ID3D11Device* device);
};

}
