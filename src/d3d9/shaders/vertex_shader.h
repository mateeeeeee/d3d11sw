#pragma once
#include <vector>
#include "d3d9/common/d3d9_headers.h"
#include "core/common/common.h"
#include "core/common/unknown_impl.h"
#include "core/shaders/parsed_shader.h"
#include "core/shaders/shader_abi.h"

namespace d3dsw {

class D3D9VertexShaderSW final : public IDirect3DVertexShader9, private UnknownBase
{
public:
    D3D9VertexShaderSW(IDirect3DDevice9* device);
    ~D3D9VertexShaderSW() override;

    HRESULT Init(const DWORD* bytecode, Usize lenBytes);

    SW_VSFn                   GetJitFn();
    const D3DSW_ParsedShader& GetReflection() const { return _reflection; }
    const std::vector<Uint8>& Bytecode()      const { return _bytecode; }

    ULONG   STDMETHODCALLTYPE AddRef() override  { return AddRefImpl(); }
    ULONG   STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) override;
    HRESULT STDMETHODCALLTYPE GetFunction(void* pData, UINT* pSizeOfData) override;

private:
    IDirect3DDevice9*    _device   = nullptr;
    std::vector<Uint8>   _bytecode;
    D3DSW_ParsedShader   _reflection;
    SW_VSFn              _jitFn    = nullptr;
    Bool                 _compiled = false;
};

}
