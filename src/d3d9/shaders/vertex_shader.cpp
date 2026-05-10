#include "d3d9/shaders/vertex_shader.h"
#include "d3d9/translate/sm3_translator.h"
#include "core/shaders/shader_jit.h"
#include <cstring>

namespace d3dsw {

D3D9VertexShaderSW::D3D9VertexShaderSW(IDirect3DDevice9* device)
    : _device(device)
{
    if (_device)
    {
        _device->AddRef();
    }
}

D3D9VertexShaderSW::~D3D9VertexShaderSW()
{
    if (_device)
    {
        _device->Release();
    }
}

HRESULT D3D9VertexShaderSW::Init(const DWORD* bytecode, Usize lenBytes)
{
    if (!bytecode || lenBytes == 0)
    {
        return D3DERR_INVALIDCALL;
    }
    _bytecode.assign(reinterpret_cast<const Uint8*>(bytecode),
                     reinterpret_cast<const Uint8*>(bytecode) + lenBytes);

    if (!SM3Parse(_bytecode.data(), _bytecode.size(), _reflection))
    {
        return D3DERR_INVALIDCALL;
    }
    if (_reflection.type != D3DSW_ShaderType::Vertex)
    {
        return D3DERR_INVALIDCALL;
    }
    return S_OK;
}

SW_VSFn D3D9VertexShaderSW::GetJitFn()
{
    if (!_compiled)
    {
        void* fn = GetShaderJIT().GetOrCompile(
            _bytecode.data(), _bytecode.size(),
            D3DSW_ShaderType::Vertex, &SM3Parse,
            D3DSW_ShaderFrontend::D3D9);
        _jitFn    = reinterpret_cast<SW_VSFn>(fn);
        _compiled = true;
    }
    return _jitFn;
}

HRESULT STDMETHODCALLTYPE D3D9VertexShaderSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }
    if (riid == IID_IUnknown || riid == IID_IDirect3DVertexShader9)
    {
        *ppv = static_cast<IDirect3DVertexShader9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE D3D9VertexShaderSW::GetDevice(IDirect3DDevice9** ppDevice)
{
    if (!ppDevice)
    {
        return D3DERR_INVALIDCALL;
    }
    *ppDevice = _device;
    if (_device)
    {
        _device->AddRef();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9VertexShaderSW::GetFunction(void* pData, UINT* pSizeOfData)
{
    if (!pSizeOfData)
    {
        return D3DERR_INVALIDCALL;
    }
    UINT n = static_cast<UINT>(_bytecode.size());
    if (!pData)
    {
        *pSizeOfData = n;
        return S_OK;
    }
    if (*pSizeOfData < n)
    {
        return D3DERR_INVALIDCALL;
    }
    std::memcpy(pData, _bytecode.data(), n);
    *pSizeOfData = n;
    return S_OK;
}

}
