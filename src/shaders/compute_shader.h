#pragma once
#include "device/device_child_impl.h"
#include "shaders/dxbc_parser.h"
#include "shaders/shader_abi.h"
#include <vector>

namespace d3d11sw {

class D3D11ComputeShaderSW final : public DeviceChildImpl<ID3D11ComputeShader>
{
public:
    explicit D3D11ComputeShaderSW(ID3D11Device* device);

    HRESULT Init(const void* bytecode, SIZE_T len);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) final;

    SW_CSFn                     GetJitFn();
    const D3D11SW_ParsedShader& GetReflection() const { return _reflection; }

private:
    std::vector<Uint8>   _bytecode;
    D3D11SW_ParsedShader _reflection;
    SW_CSFn              _jitFn    = nullptr;
    Bool                 _compiled = false;
};

}
