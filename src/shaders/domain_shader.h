#pragma once
#include "common/common.h"
#include "common/device_child_impl.h"

class MarsDomainShader : public DeviceChildImpl<ID3D11DomainShader, ID3D11DeviceChild>
{
public:
    MarsDomainShader(ID3D11Device* device);
};
