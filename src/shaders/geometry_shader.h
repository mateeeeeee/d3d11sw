#pragma once
#include <vector>
#include "device/device_child_impl.h"
#include "shaders/dxbc_parser.h"
#include "shaders/shader_abi.h"

namespace d3d11sw {

struct SODecl
{
    Uint8  stream;
    Char   semanticName[64];
    Uint32 semanticIndex;
    Uint8  startComponent;
    Uint8  componentCount;
    Uint8  outputSlot;
};

class D3D11GeometryShaderSW final : public DeviceChildImpl<ID3D11GeometryShader>
{
public:
    explicit D3D11GeometryShaderSW(ID3D11Device* device);

    HRESULT Init(const void* bytecode, SIZE_T len);
    HRESULT InitWithStreamOutput(const void* bytecode, SIZE_T len,
                                 const D3D11_SO_DECLARATION_ENTRY* pSODeclaration, UINT numEntries,
                                 const UINT* pBufferStrides, UINT numStrides,
                                 UINT rasterizedStream);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    SW_GSFn                     GetJitFn();
    const D3D11SW_ParsedShader& GetReflection() const { return _reflection; }

    Bool                        HasSO() const { return _hasSO; }
    const std::vector<SODecl>&  GetSODecls() const { return _soDecls; }
    const Uint32*               GetSOStrides() const { return _soStrides; }
    Uint32                      GetRasterizedStream() const { return _rasterizedStream; }

private:
    std::vector<Uint8>   _bytecode;
    D3D11SW_ParsedShader _reflection;
    SW_GSFn              _jitFn    = nullptr;
    Bool                 _compiled = false;

    std::vector<SODecl>  _soDecls;
    Uint32               _soStrides[4] = {};
    Uint32               _rasterizedStream = 0;
    Bool                 _hasSO = false;
};

}
