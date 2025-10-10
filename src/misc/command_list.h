#pragma once

#include "common/device_child_impl.h"

class MarsCommandList : public DeviceChildImpl<ID3D11CommandList, ID3D11DeviceChild>
{
public:
    MarsCommandList(ID3D11Device* device);

    UINT STDMETHODCALLTYPE GetContextFlags() override;
};
