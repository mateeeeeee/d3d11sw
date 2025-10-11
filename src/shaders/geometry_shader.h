#pragma once
#include "common/common.h"
#include "common/device_child_impl.h"

namespace d3d11sw {


class D3D11GeometryShaderSW final : public DeviceChildImpl<ID3D11GeometryShader>
{
public:
    explicit D3D11GeometryShaderSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
};

}
