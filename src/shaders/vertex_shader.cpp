#include "shaders/vertex_shader.h"
#include "shaders/shader_jit.h"

namespace d3d11sw {

D3D11VertexShaderSW::D3D11VertexShaderSW(ID3D11Device* device) : DeviceChildImpl(device) {}

HRESULT D3D11VertexShaderSW::Init(const void* bytecode, SIZE_T len)
{
    _bytecode.assign(static_cast<const Uint8*>(bytecode),
                     static_cast<const Uint8*>(bytecode) + len);

    DXBCParser parser;
    if (!parser.Parse(_bytecode.data(), _bytecode.size(), _reflection))
    {
        return E_INVALIDARG;
    }
    return S_OK;
}

SW_VSFn D3D11VertexShaderSW::GetJitFn()
{
    if (!_compiled)
    {
        void* fn = GetShaderJIT().GetOrCompile(_bytecode.data(), _bytecode.size(), D3D11SW_ShaderType::Vertex);
        _jitFn = reinterpret_cast<SW_VSFn>(fn);
        _compiled = true;
    }
    return _jitFn;
}

HRESULT STDMETHODCALLTYPE D3D11VertexShaderSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) 
    { 
        return E_POINTER; 
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11VertexShader))
    {
        *ppv = static_cast<ID3D11VertexShader*>(this);
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
