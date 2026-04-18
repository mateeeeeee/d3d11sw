#include "shaders/hull_shader.h"
#include "shaders/shader_jit.h"
#include <format>

namespace d3d11sw {

D3D11HullShaderSW::D3D11HullShaderSW(ID3D11Device* device) : DeviceChildImpl(device) {}

HRESULT D3D11HullShaderSW::Init(const void* bytecode, SIZE_T len)
{
    _bytecode.assign(static_cast<const Uint8*>(bytecode),
                     static_cast<const Uint8*>(bytecode) + len);

    if (!DXBCParse(_bytecode.data(), _bytecode.size(), _reflection))
    {
        return E_INVALIDARG;
    }
    return S_OK;
}

void D3D11HullShaderSW::Compile()
{
    if (_compiled)
    {
        return;
    }
    GetShaderJIT().GetOrCompile(_bytecode.data(), _bytecode.size(), D3D11SW_ShaderType::Hull);

    if (!_reflection.hsControlPointPhase.instrs.empty())
    {
        _cpFn = reinterpret_cast<SW_HSCPFn>(
            GetShaderJIT().FindSymbol(_bytecode.data(), _bytecode.size(), "HS_CP"));
    }

    _forkFns.resize(_reflection.hsForkPhases.size(), nullptr);
    for (Uint32 i = 0; i < _reflection.hsForkPhases.size(); ++i)
    {
        auto name = std::format("HS_Fork{}", i);
        _forkFns[i] = reinterpret_cast<SW_HSForkJoinFn>(
            GetShaderJIT().FindSymbol(_bytecode.data(), _bytecode.size(), name.c_str()));
    }

    _joinFns.resize(_reflection.hsJoinPhases.size(), nullptr);
    for (Uint32 i = 0; i < _reflection.hsJoinPhases.size(); ++i)
    {
        auto name = std::format("HS_Join{}", i);
        _joinFns[i] = reinterpret_cast<SW_HSForkJoinFn>(
            GetShaderJIT().FindSymbol(_bytecode.data(), _bytecode.size(), name.c_str()));
    }
    _compiled = true;
}

SW_HSCPFn D3D11HullShaderSW::GetCPFn()
{
    Compile();
    return _cpFn;
}

SW_HSForkJoinFn D3D11HullShaderSW::GetForkFn(Uint32 index)
{
    Compile();
    return index < _forkFns.size() ? _forkFns[index] : nullptr;
}

SW_HSForkJoinFn D3D11HullShaderSW::GetJoinFn(Uint32 index)
{
    Compile();
    return index < _joinFns.size() ? _joinFns[index] : nullptr;
}

HRESULT STDMETHODCALLTYPE D3D11HullShaderSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11HullShader))
    {
        *ppv = static_cast<ID3D11HullShader*>(this);
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
