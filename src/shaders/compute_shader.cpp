#include "shaders/compute_shader.h"
#include "shaders/shader_jit.h"

namespace d3d11sw {

D3D11ComputeShaderSW::D3D11ComputeShaderSW(ID3D11Device* device) : DeviceChildImpl(device) {}

HRESULT D3D11ComputeShaderSW::Init(const void* bytecode, SIZE_T len)
{
    _bytecode.assign(static_cast<const Uint8*>(bytecode),
                     static_cast<const Uint8*>(bytecode) + len);

    DXBCParser parser;
    if (!parser.ParseReflection(_bytecode.data(), _bytecode.size(), _reflection))
    {
        return E_INVALIDARG;
    }
    return S_OK;
}

SW_CSFn D3D11ComputeShaderSW::GetJitFn()
{
    if (!_compiled)
    {
        void* fn = GetShaderJIT().GetOrCompile(
            _bytecode.data(), _bytecode.size(), D3D11SW_ShaderType::Compute);
        _jitFn    = reinterpret_cast<SW_CSFn>(fn);
        _compiled = true;
    }
    return _jitFn;
}

HRESULT STDMETHODCALLTYPE D3D11ComputeShaderSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) { return E_POINTER; }
    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11ComputeShader))
    {
        *ppv = static_cast<ID3D11ComputeShader*>(this);
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
