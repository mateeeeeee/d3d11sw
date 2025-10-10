#pragma once

#include "common/device_child_impl.h"

class MarsInputLayout : public DeviceChildImpl<ID3D11InputLayout, ID3D11DeviceChild>
{
public:
    MarsInputLayout(ID3D11Device* device);
};
