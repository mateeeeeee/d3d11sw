#include "shaders/domain_shader.h"
#include "shaders/shader_jit.h"

namespace d3d11sw {

D3D11DomainShaderSW::D3D11DomainShaderSW(ID3D11Device* device) : DeviceChildImpl(device) {}

HRESULT D3D11DomainShaderSW::Init(const void* bytecode, SIZE_T len)
{
    _bytecode.assign(static_cast<const Uint8*>(bytecode),
                     static_cast<const Uint8*>(bytecode) + len);

    if (!DXBCParse(_bytecode.data(), _bytecode.size(), _reflection))
    {
        return E_INVALIDARG;
    }
    return S_OK;
}

SW_DSFn D3D11DomainShaderSW::GetJitFn()
{
    if (!_compiled)
    {
        void* fn = GetShaderJIT().GetOrCompile(_bytecode.data(), _bytecode.size(), D3D11SW_ShaderType::Domain);
        _jitFn = reinterpret_cast<SW_DSFn>(fn);
        _compiled = true;
    }
    return _jitFn;
}

HRESULT STDMETHODCALLTYPE D3D11DomainShaderSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11DomainShader))
    {
        *ppv = static_cast<ID3D11DomainShader*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceChild))
    {
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

}
