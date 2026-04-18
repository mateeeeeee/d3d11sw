#pragma once
#include "common/device_child_impl.h"
#include "shaders/dxbc_parser.h"
#include "shaders/shader_abi.h"
#include <vector>

namespace d3d11sw {

class D3D11HullShaderSW final : public DeviceChildImpl<ID3D11HullShader>
{
public:
    explicit D3D11HullShaderSW(ID3D11Device* device);

    HRESULT Init(const void* bytecode, SIZE_T len);
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    SW_HSCPFn                   GetCPFn();
    SW_HSForkJoinFn             GetForkFn(Uint32 index);
    SW_HSForkJoinFn             GetJoinFn(Uint32 index);
    const D3D11SW_ParsedShader& GetReflection() const { return _reflection; }

    Uint32 GetInputCPCount() const    { return _reflection.hsInputControlPointCount; }
    Uint32 GetOutputCPCount() const   { return _reflection.hsOutputControlPointCount; }
    Uint32 GetDomain() const          { return _reflection.hsDomain; }
    Uint32 GetPartitioning() const    { return _reflection.hsPartitioning; }
    Uint32 GetOutputPrimitive() const { return _reflection.hsOutputPrimitive; }
    Float  GetMaxTessFactor() const   { return _reflection.hsMaxTessFactor; }
    Uint32 GetForkPhaseCount() const  { return static_cast<Uint32>(_reflection.hsForkPhases.size()); }
    Uint32 GetJoinPhaseCount() const  { return static_cast<Uint32>(_reflection.hsJoinPhases.size()); }

private:
    std::vector<Uint8>          _bytecode;
    D3D11SW_ParsedShader        _reflection;
    SW_HSCPFn                   _cpFn = nullptr;
    std::vector<SW_HSForkJoinFn> _forkFns;
    std::vector<SW_HSForkJoinFn> _joinFns;
    Bool                        _compiled = false;

private:
    void Compile();
};

}
