#pragma once
#include "common/common.h"
#include "common/device_child_impl.h"

namespace d3d11sw {


class D3D11DomainShaderSW final : public DeviceChildImpl<ID3D11DomainShader>
{
public:
    explicit D3D11DomainShaderSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
};

}
