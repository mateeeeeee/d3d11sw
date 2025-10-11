#pragma once
#include "common/common.h"
#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11GeometryShaderSW : public DeviceChildImpl<ID3D11GeometryShader>
{
public:
    explicit Direct3D11GeometryShaderSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) final;
};

}
