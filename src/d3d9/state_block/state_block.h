#pragma once
#include "d3d9/common/d3d9_headers.h"
#include "core/common/common.h"
#include "core/common/unknown_impl.h"

namespace d3dsw {

class D3D9DeviceSW;
class D3D9VertexShaderSW;
class D3D9PixelShaderSW;
class D3D9VertexDeclarationSW;
class D3D9VertexBufferSW;
class D3D9IndexBufferSW;

class D3D9StateBlockSW final : public IDirect3DStateBlock9, private UnknownBase
{
    static constexpr Uint MaxRTs      = 4;
    static constexpr Uint MaxStreams   = 16;
    static constexpr Uint MaxStages   = 8;
    static constexpr Uint MaxLights   = 8;
    static constexpr Uint MaxClipPlanes = 6;

public:
    explicit D3D9StateBlockSW(D3D9DeviceSW* device);
    ~D3D9StateBlockSW() override;

    void CaptureAll(D3D9DeviceSW* device);
    void MarkAllChanged();

    void RecordRenderState(D3DRENDERSTATETYPE state, DWORD value);
    void RecordTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix);
    void RecordTexture(DWORD stage, IDirect3DBaseTexture9* texture);
    void RecordTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value);
    void RecordSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value);
    void RecordVertexShader(D3D9VertexShaderSW* vs);
    void RecordPixelShader(D3D9PixelShaderSW* ps);
    void RecordVertexDeclaration(D3D9VertexDeclarationSW* decl);
    void RecordFVF(DWORD fvf);
    void RecordIndices(D3D9IndexBufferSW* ib);
    void RecordStreamSource(UINT stream, D3D9VertexBufferSW* buffer, UINT offset, UINT stride);
    void RecordStreamSourceFreq(UINT stream, UINT freq);
    void RecordViewport(const D3DVIEWPORT9* viewport);
    void RecordScissorRect(const RECT* rect);
    void RecordMaterial(const D3DMATERIAL9* material);
    void RecordLight(DWORD index, const D3DLIGHT9* light);
    void RecordLightEnable(DWORD index, BOOL enable);
    void RecordClipPlane(DWORD index, const float* plane);
    void RecordVSConstantF(UINT reg, const float* data, UINT count);
    void RecordVSConstantI(UINT reg, const int* data, UINT count);
    void RecordVSConstantB(UINT reg, const BOOL* data, UINT count);
    void RecordPSConstantF(UINT reg, const float* data, UINT count);
    void RecordPSConstantI(UINT reg, const int* data, UINT count);
    void RecordPSConstantB(UINT reg, const BOOL* data, UINT count);

    ULONG   STDMETHODCALLTYPE AddRef()  override { return AddRefImpl(); }
    ULONG   STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) override;
    HRESULT STDMETHODCALLTYPE Capture() override;
    HRESULT STDMETHODCALLTYPE Apply() override;

private:
    D3D9DeviceSW* _device = nullptr;
    Bool          _valid  = false;

    Uint32      _changedRS[7]                    = {};
    Uint32      _changedTransforms[16]           = {};
    Uint16      _changedSamplerState[MaxStages]  = {};
    Uint64      _changedTSS[MaxStages]           = {};
    Uint8       _changedTexture                  = 0;
    Uint8       _changedLight                    = 0;
    Uint8       _changedLightEnable              = 0;
    Uint32      _changedVSConstF[8]              = {};
    Uint32      _changedPSConstF[7]              = {};
    Uint16      _changedVSConstI                 = 0;
    Uint16      _changedVSConstB                 = 0;
    Uint16      _changedPSConstI                 = 0;
    Uint16      _changedPSConstB                 = 0;
    Uint16      _changedStreams                  = 0;
    Uint16      _changedStreamFreq               = 0;
    Uint8       _changedClipPlane                = 0;

    Bool        _changedViewport                 = false;
    Bool        _changedScissor                  = false;
    Bool        _changedMaterial                 = false;
    Bool        _changedFVF                      = false;
    Bool        _changedIndices                  = false;
    Bool        _changedVertexDecl               = false;
    Bool        _changedVS                       = false;
    Bool        _changedPS                       = false;

    DWORD        _rs[210]                        = {};
    D3DMATRIX    _transforms[512]                = {};
    DWORD        _samplerStates[MaxStages][14]   = {};
    DWORD        _tss[MaxStages][33]             = {};
    D3DMATERIAL9 _material                       = {};
    D3DLIGHT9    _lights[MaxLights]              = {};
    BOOL         _lightEnabled[MaxLights]        = {};
    Float        _vsConstF[256][4]               = {};
    Int32        _vsConstI[16][4]                = {};
    Uint32       _vsConstB[16]                   = {};
    Float        _psConstF[224][4]               = {};
    Int32        _psConstI[16][4]                = {};
    Uint32       _psConstB[16]                   = {};
    DWORD        _fvf                            = 0;
    D3DVIEWPORT9 _viewport                       = {};
    RECT         _scissor                        = {};
    Float        _clipPlanes[MaxClipPlanes][4]   = {};

    struct StreamBinding
    {
        D3D9VertexBufferSW* buffer;
        Uint offset;
        Uint stride;
    };
    StreamBinding _streams[MaxStreams]            = {};
    Uint          _streamFreq[MaxStreams]         = {};

    IDirect3DBaseTexture9*   _textures[MaxStages] = {};
    D3D9VertexShaderSW*      _vs                  = nullptr;
    D3D9PixelShaderSW*       _ps                  = nullptr;
    D3D9VertexDeclarationSW* _vertexDecl          = nullptr;
    D3D9IndexBufferSW*       _indices             = nullptr;

private:
    void ReleaseRefs();
};

}
