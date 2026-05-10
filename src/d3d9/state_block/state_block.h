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
    static constexpr UINT kMaxRTs      = 4;
    static constexpr UINT kMaxStreams  = 16;
    static constexpr UINT kMaxStages   = 8;
    static constexpr UINT kMaxLights   = 8;
    static constexpr UINT kMaxClipPl   = 6;
    
public:
    explicit D3D9StateBlockSW(D3D9DeviceSW* device);
    ~D3D9StateBlockSW() override;

    void Capture(D3D9DeviceSW* device);

    ULONG   STDMETHODCALLTYPE AddRef()  override { return AddRefImpl(); }
    ULONG   STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) override;
    HRESULT STDMETHODCALLTYPE Capture() override;
    HRESULT STDMETHODCALLTYPE Apply() override;

private:

    D3D9DeviceSW* _device = nullptr;
    Bool          _valid  = false;   

    DWORD       _rs[210]              = {};
    D3DMATRIX   _transforms[512]      = {};
    DWORD       _samplerStates[kMaxStages][14] = {};
    DWORD       _tss[kMaxStages][33]  = {};
    D3DMATERIAL9 _material            = {};
    D3DLIGHT9   _lights[kMaxLights]   = {};
    BOOL        _lightEnabled[kMaxLights] = {};
    Float       _vsConstF[256][4]     = {};
    Int32       _vsConstI[16][4]      = {};
    Uint32      _vsConstB[16]         = {};
    Float       _psConstF[224][4]     = {};
    Int32       _psConstI[16][4]      = {};
    Uint32      _psConstB[16]         = {};
    DWORD       _fvf                  = 0;
    D3DVIEWPORT9 _viewport            = {};
    RECT        _scissor              = {};
    BOOL        _scissorEnabled       = FALSE;
    Float       _clipPlanes[kMaxClipPl][4] = {};
    D3DCLIPSTATUS9 _clipStatus        = {};

    struct StreamBinding { D3D9VertexBufferSW* buffer; UINT offset; UINT stride; };
    StreamBinding _streams[kMaxStreams] = {};
    Uint          _streamFreq[kMaxStreams] = {};

    IDirect3DBaseTexture9*  _textures[kMaxStages] = {};
    D3D9VertexShaderSW*     _vs                   = nullptr;
    D3D9PixelShaderSW*      _ps                   = nullptr;
    D3D9VertexDeclarationSW* _vertexDecl           = nullptr;
    D3D9IndexBufferSW*      _indices               = nullptr;

private:
    void ReleaseRefs();
};

}
