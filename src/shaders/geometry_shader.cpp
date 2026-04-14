#include "shaders/geometry_shader.h"
#include "shaders/shader_jit.h"
#include <cstring>

namespace d3d11sw {

D3D11GeometryShaderSW::D3D11GeometryShaderSW(ID3D11Device* device) : DeviceChildImpl(device) {}

HRESULT D3D11GeometryShaderSW::Init(const void* bytecode, SIZE_T len)
{
    _bytecode.assign(static_cast<const Uint8*>(bytecode),
                     static_cast<const Uint8*>(bytecode) + len);

    if (!DXBCParse(_bytecode.data(), _bytecode.size(), _reflection))
    {
        return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT D3D11GeometryShaderSW::InitWithStreamOutput(
    const void* bytecode, SIZE_T len,
    const D3D11_SO_DECLARATION_ENTRY* pSODeclaration, UINT numEntries,
    const UINT* pBufferStrides, UINT numStrides,
    UINT rasterizedStream)
{
    if (bytecode && len > 0)
    {
        HRESULT hr = Init(bytecode, len);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    _hasSO = true;
    _rasterizedStream = rasterizedStream;
    for (Uint i = 0; i < numStrides && i < 4; ++i)
    {
        _soStrides[i] = pBufferStrides[i];
    }

    _soDecls.reserve(numEntries);
    for (Uint i = 0; i < numEntries; ++i)
    {
        SODecl d{};
        d.stream         = static_cast<Uint8>(pSODeclaration[i].Stream);
        d.semanticIndex  = pSODeclaration[i].SemanticIndex;
        d.startComponent = static_cast<Uint8>(pSODeclaration[i].StartComponent);
        d.componentCount = static_cast<Uint8>(pSODeclaration[i].ComponentCount);
        d.outputSlot     = static_cast<Uint8>(pSODeclaration[i].OutputSlot);
        if (pSODeclaration[i].SemanticName)
        {
            Usize nameLen = std::strlen(pSODeclaration[i].SemanticName);
            if (nameLen > 63) 
            { 
                nameLen = 63;
            }

            std::memcpy(d.semanticName, pSODeclaration[i].SemanticName, nameLen);
            d.semanticName[nameLen] = '\0';
        }
        _soDecls.push_back(d);
    }

    return S_OK;
}

SW_GSFn D3D11GeometryShaderSW::GetJitFn()
{
    if (!_compiled)
    {
        void* fn = GetShaderJIT().GetOrCompile(_bytecode.data(), _bytecode.size(), D3D11SW_ShaderType::Geometry);
        _jitFn = reinterpret_cast<SW_GSFn>(fn);
        _compiled = true;
    }
    return _jitFn;
}

HRESULT STDMETHODCALLTYPE D3D11GeometryShaderSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11GeometryShader))
    {
        *ppv = static_cast<ID3D11GeometryShader*>(this);
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
