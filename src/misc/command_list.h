#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11CommandListSW : public DeviceChildImpl<ID3D11CommandList, ID3D11DeviceChild>
{
public:
    explicit Direct3D11CommandListSW(ID3D11Device* device);

    UINT STDMETHODCALLTYPE GetContextFlags() override;
};

}
