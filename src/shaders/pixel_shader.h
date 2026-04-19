#pragma once
#include <vector>
#include "device/device_child_impl.h"
#include "shaders/dxbc_parser.h"
#include "shaders/shader_abi.h"

namespace d3d11sw {

class D3D11PixelShaderSW final : public DeviceChildImpl<ID3D11PixelShader>
{
public:
    explicit D3D11PixelShaderSW(ID3D11Device* device);

    HRESULT Init(const void* bytecode, SIZE_T len);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    SW_PSFn                     GetJitFn();
    const D3D11SW_ParsedShader& GetReflection() const { return _reflection; }

private:
    std::vector<Uint8>   _bytecode;
    D3D11SW_ParsedShader _reflection;
    SW_PSFn              _jitFn    = nullptr;
    Bool                 _compiled = false;
};

}
