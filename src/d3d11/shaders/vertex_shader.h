#pragma once
#include <vector>
#include "d3d11/common/d3d11_headers.h"
#include "d3d11/device/device_child_impl.h"
#include "core/shaders/parsed_shader.h"
#include "core/shaders/shader_abi.h"

namespace d3dsw {

class D3D11VertexShaderSW final : public DeviceChildImpl<ID3D11VertexShader>
{
public:
    explicit D3D11VertexShaderSW(ID3D11Device* device);

    HRESULT Init(const void* bytecode, SIZE_T len);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    SW_VSFn                     GetJitFn();
    const D3DSW_ParsedShader& GetReflection() const { return _reflection; }

private:
    std::vector<Uint8>   _bytecode;
    D3DSW_ParsedShader _reflection;
    SW_VSFn              _jitFn    = nullptr;
    Bool                 _compiled = false;
};

}
