#pragma once
#include <map>
#include "d3d9/common/d3d9_headers.h"
#include "d3d9/context/draw_state.h"
#include "d3d9/ff/ff_synthesizer.h"
#include "core/common/common.h"
#include "core/common/unknown_impl.h"
#include "core/pipeline/sw_pipeline_state.h"
#include "core/rasterizer/rasterizer.h"

namespace d3dsw {

class D3D9SW;
class D3D9SurfaceSW;
class D3D9SwapChainSW;
class D3D9VertexBufferSW;
class D3D9IndexBufferSW;
class D3D9VertexDeclarationSW;
class D3D9VertexShaderSW;
class D3D9PixelShaderSW;
class D3D9StateBlockSW;

class D3D9DeviceSW final : public IDirect3DDevice9Ex, private UnknownBase
{
public:
    D3D9DeviceSW(D3D9SW* parent, const D3DPRESENT_PARAMETERS& params, HWND focusWindow, Bool isEx = false);
    ~D3D9DeviceSW() override;

    void SnapshotDrawState(D3D9SW_DRAW_STATE& out, D3DPRIMITIVETYPE primitiveType) const;

    ULONG   STDMETHODCALLTYPE AddRef() override  { return AddRefImpl(); }
    ULONG   STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE TestCooperativeLevel() override;
    UINT    STDMETHODCALLTYPE GetAvailableTextureMem() override;
    HRESULT STDMETHODCALLTYPE EvictManagedResources() override;
    HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D9** ppD3D9) override;
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS9* pCaps) override;
    HRESULT STDMETHODCALLTYPE GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) override;
    HRESULT STDMETHODCALLTYPE SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) override;
    void    STDMETHODCALLTYPE SetCursorPosition(int X, int Y, DWORD Flags) override;
    BOOL    STDMETHODCALLTYPE ShowCursor(BOOL bShow) override;
    HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) override;
    HRESULT STDMETHODCALLTYPE GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) override;
    UINT    STDMETHODCALLTYPE GetNumberOfSwapChains() override;
    HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) override;
    HRESULT STDMETHODCALLTYPE Present(const RECT* src_rect, const RECT* dst_rect, HWND dst_window_override, const RGNDATA* dirty_region) override;
    HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) override;
    HRESULT STDMETHODCALLTYPE GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) override;
    HRESULT STDMETHODCALLTYPE SetDialogBoxMode(BOOL bEnableDialogs) override;
    void    STDMETHODCALLTYPE SetGammaRamp(UINT swapchain_idx, DWORD flags, const D3DGAMMARAMP* ramp) override;
    void    STDMETHODCALLTYPE GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp) override;
    HRESULT STDMETHODCALLTYPE CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) override;
    HRESULT STDMETHODCALLTYPE CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) override;
    HRESULT STDMETHODCALLTYPE CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle) override;
    HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) override;
    HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle) override;
    HRESULT STDMETHODCALLTYPE CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override;
    HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override;
    HRESULT STDMETHODCALLTYPE UpdateSurface(IDirect3DSurface9* src_surface, const RECT* src_rect, IDirect3DSurface9* dst_surface, const POINT* dst_point) override;
    HRESULT STDMETHODCALLTYPE UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) override;
    HRESULT STDMETHODCALLTYPE GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) override;
    HRESULT STDMETHODCALLTYPE GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9* pDestSurface) override;
    HRESULT STDMETHODCALLTYPE StretchRect(IDirect3DSurface9* src_surface, const RECT* src_rect, IDirect3DSurface9* dst_surface, const RECT* dst_rect, D3DTEXTUREFILTERTYPE filter) override;
    HRESULT STDMETHODCALLTYPE ColorFill(IDirect3DSurface9* surface, const RECT* rect, D3DCOLOR color) override;
    HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override;
    HRESULT STDMETHODCALLTYPE SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) override;
    HRESULT STDMETHODCALLTYPE GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) override;
    HRESULT STDMETHODCALLTYPE SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil) override;
    HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface) override;
    HRESULT STDMETHODCALLTYPE BeginScene() override;
    HRESULT STDMETHODCALLTYPE EndScene() override;
    HRESULT STDMETHODCALLTYPE Clear(DWORD rect_count, const D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil) override;
    HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix) override;
    HRESULT STDMETHODCALLTYPE GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) override;
    HRESULT STDMETHODCALLTYPE MultiplyTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix) override;
    HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT9* viewport) override;
    HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT9* pViewport) override;
    HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL9* material) override;
    HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL9* pMaterial) override;
    HRESULT STDMETHODCALLTYPE SetLight(DWORD index, const D3DLIGHT9* light) override;
    HRESULT STDMETHODCALLTYPE GetLight(DWORD Index, D3DLIGHT9*) override;
    HRESULT STDMETHODCALLTYPE LightEnable(DWORD Index, BOOL Enable) override;
    HRESULT STDMETHODCALLTYPE GetLightEnable(DWORD Index, BOOL* pEnable) override;
    HRESULT STDMETHODCALLTYPE SetClipPlane(DWORD index, const float* plane) override;
    HRESULT STDMETHODCALLTYPE GetClipPlane(DWORD Index, float* pPlane) override;
    HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) override;
    HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) override;
    HRESULT STDMETHODCALLTYPE CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) override;
    HRESULT STDMETHODCALLTYPE BeginStateBlock() override;
    HRESULT STDMETHODCALLTYPE EndStateBlock(IDirect3DStateBlock9** ppSB) override;
    HRESULT STDMETHODCALLTYPE SetClipStatus(const D3DCLIPSTATUS9* clip_status) override;
    HRESULT STDMETHODCALLTYPE GetClipStatus(D3DCLIPSTATUS9* pClipStatus) override;
    HRESULT STDMETHODCALLTYPE GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture) override;
    HRESULT STDMETHODCALLTYPE SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) override;
    HRESULT STDMETHODCALLTYPE GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) override;
    HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) override;
    HRESULT STDMETHODCALLTYPE GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) override;
    HRESULT STDMETHODCALLTYPE SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) override;
    HRESULT STDMETHODCALLTYPE ValidateDevice(DWORD* pNumPasses) override;
    HRESULT STDMETHODCALLTYPE SetPaletteEntries(UINT palette_idx, const PALETTEENTRY* entries) override;
    HRESULT STDMETHODCALLTYPE GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries) override;
    HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(UINT PaletteNumber) override;
    HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(UINT* PaletteNumber) override;
    HRESULT STDMETHODCALLTYPE SetScissorRect(const RECT* rect) override;
    HRESULT STDMETHODCALLTYPE GetScissorRect(RECT* pRect) override;
    HRESULT STDMETHODCALLTYPE SetSoftwareVertexProcessing(BOOL bSoftware) override;
    BOOL STDMETHODCALLTYPE GetSoftwareVertexProcessing() override;
    HRESULT STDMETHODCALLTYPE SetNPatchMode(float nSegments) override;
    float   STDMETHODCALLTYPE GetNPatchMode() override;
    HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override;
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override;
    HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE primitive_type, UINT primitive_count, const void* data, UINT stride) override;
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE primitive_type, UINT min_vertex_idx, UINT vertex_count, UINT primitive_count, const void* index_data, D3DFORMAT index_format, const void* data, UINT stride) override;
    HRESULT STDMETHODCALLTYPE ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE CreateVertexDeclaration(const D3DVERTEXELEMENT9* elements, IDirect3DVertexDeclaration9** declaration) override;
    HRESULT STDMETHODCALLTYPE SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl) override;
    HRESULT STDMETHODCALLTYPE GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) override;
    HRESULT STDMETHODCALLTYPE SetFVF(DWORD FVF) override;
    HRESULT STDMETHODCALLTYPE GetFVF(DWORD* pFVF) override;
    HRESULT STDMETHODCALLTYPE CreateVertexShader(const DWORD* byte_code, IDirect3DVertexShader9** shader) override;
    HRESULT STDMETHODCALLTYPE SetVertexShader(IDirect3DVertexShader9* pShader) override;
    HRESULT STDMETHODCALLTYPE GetVertexShader(IDirect3DVertexShader9** ppShader) override;
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantF(UINT reg_idx, const float* data, UINT count) override;
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) override;
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantI(UINT reg_idx, const int* data, UINT count) override;
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) override;
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantB(UINT reg_idx, const BOOL* data, UINT count) override;
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) override;
    HRESULT STDMETHODCALLTYPE SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) override;
    HRESULT STDMETHODCALLTYPE GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* OffsetInBytes, UINT* pStride) override;
    HRESULT STDMETHODCALLTYPE SetStreamSourceFreq(UINT StreamNumber, UINT Divider) override;
    HRESULT STDMETHODCALLTYPE GetStreamSourceFreq(UINT StreamNumber, UINT* Divider) override;
    HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer9* pIndexData) override;
    HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer9** ppIndexData) override;
    HRESULT STDMETHODCALLTYPE CreatePixelShader(const DWORD* byte_code, IDirect3DPixelShader9** shader) override;
    HRESULT STDMETHODCALLTYPE SetPixelShader(IDirect3DPixelShader9* pShader) override;
    HRESULT STDMETHODCALLTYPE GetPixelShader(IDirect3DPixelShader9** ppShader) override;
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantF(UINT reg_idx, const float* data, UINT count) override;
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) override;
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantI(UINT reg_idx, const int* data, UINT count) override;
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) override;
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantB(UINT reg_idx, const BOOL* data, UINT count) override;
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) override;
    HRESULT STDMETHODCALLTYPE DrawRectPatch(UINT handle, const float* segment_count, const D3DRECTPATCH_INFO* patch_info) override;
    HRESULT STDMETHODCALLTYPE DrawTriPatch(UINT handle, const float* segment_count, const D3DTRIPATCH_INFO* patch_info) override;
    HRESULT STDMETHODCALLTYPE DeletePatch(UINT Handle) override;
    HRESULT STDMETHODCALLTYPE CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) override;

    // IDirect3DDevice9Ex
    HRESULT STDMETHODCALLTYPE SetConvolutionMonoKernel(UINT width, UINT height, float* rows, float* columns) override;
    HRESULT STDMETHODCALLTYPE ComposeRects(IDirect3DSurface9* pSrc, IDirect3DSurface9* pDst, IDirect3DVertexBuffer9* pSrcRectDescs, UINT NumRects, IDirect3DVertexBuffer9* pDstRectDescs, D3DCOMPOSERECTSOP Operation, INT Xoffset, INT Yoffset) override;
    HRESULT STDMETHODCALLTYPE PresentEx(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion, DWORD dwFlags) override;
    HRESULT STDMETHODCALLTYPE GetGPUThreadPriority(INT* pPriority) override;
    HRESULT STDMETHODCALLTYPE SetGPUThreadPriority(INT Priority) override;
    HRESULT STDMETHODCALLTYPE WaitForVBlank(UINT iSwapChain) override;
    HRESULT STDMETHODCALLTYPE CheckResourceResidency(IDirect3DResource9** pResourceArray, UINT32 NumResources) override;
    HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency) override;
    HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(UINT* pMaxLatency) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceState(HWND hDestinationWindow) override;
    HRESULT STDMETHODCALLTYPE CreateRenderTargetEx(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage) override;
    HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurfaceEx(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage) override;
    HRESULT STDMETHODCALLTYPE CreateDepthStencilSurfaceEx(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage) override;
    HRESULT STDMETHODCALLTYPE ResetEx(D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode) override;
    HRESULT STDMETHODCALLTYPE GetDisplayModeEx(UINT iSwapChain, D3DDISPLAYMODEEX* pMode, D3DDISPLAYROTATION* pRotation) override;

private:
    friend class D3D9StateBlockSW;

    struct StreamBinding
    {
        D3D9VertexBufferSW* buffer = nullptr;
        UINT                offset = 0;
        UINT                stride = 0;
    };

    D3D9SW*                    _parent            = nullptr;
    Bool                       _isEx              = false;
    D3D9SwapChainSW*           _implicitSwapChain = nullptr;
    D3D9SurfaceSW*             _renderTargets[SW_D3D9_MAX_RTS] = {};
    D3D9SurfaceSW*             _currentDSV        = nullptr;
    StreamBinding              _streams[SW_D3D9_MAX_STREAMS] = {};
    UINT                       _streamFreq[SW_D3D9_MAX_STREAMS] = {};
    D3D9IndexBufferSW*         _indices           = nullptr;
    D3D9VertexDeclarationSW*   _vertexDecl        = nullptr;
    DWORD                      _fvf               = 0;
    D3D9VertexShaderSW*        _vs                = nullptr;
    D3D9PixelShaderSW*         _ps                = nullptr;

    // SM3 constant register banks 
    Float                      _vsConstF[256][4]  = {};
    Int32                      _vsConstI[16][4]   = {};
    Uint32                     _vsConstB[16]      = {}; 
    Float                      _psConstF[224][4]  = {};
    Int32                      _psConstI[16][4]   = {};
    Uint32                     _psConstB[16]      = {};
    D3DVIEWPORT9               _viewport          = {};
    RECT                       _scissor           = {};
    BOOL                       _scissorEnabled    = FALSE;
    Bool                       _inScene           = false;
    SWRasterizer               _rasterizer;
    SWPipelineStatistics       _stats             = {};

    //Render states, indexed by D3DRENDERSTATETYPE
    DWORD                      _rs[210]           = {};

    // Transform matrices — D3DTS_VIEW=2, D3DTS_PROJECTION=3,
    // D3DTS_TEXTURE0..7=16..23, D3DTS_WORLDMATRIX(0..255)=256..511.
    D3DMATRIX                  _transforms[512]   = {};

    // Texture stages (0–7 = PS, set via SetTexture; VS stages 256–259 deferred)
    static constexpr UINT      SW_D3D9_MAX_TEXTURE_STAGES = 8;
    IDirect3DBaseTexture9*     _textures[SW_D3D9_MAX_TEXTURE_STAGES] = {};

    // Sampler states (0–7 = PS samplers, D3DSAMP_* indices 1–13)
    DWORD                      _samplerStates[SW_D3D9_MAX_TEXTURE_STAGES][14] = {};

    // Texture stage states (stages 0–7, indices D3DTSS_* 1–32)
    DWORD                      _tss[SW_D3D9_MAX_TEXTURE_STAGES][33] = {};

    // Fixed-function state (storage only; synthesizer is v1+)
    D3DMATERIAL9               _material            = {};
    static constexpr UINT      SW_D3D9_MAX_LIGHTS   = 8;
    D3DLIGHT9                  _lights[SW_D3D9_MAX_LIGHTS] = {};
    BOOL                       _lightEnabled[SW_D3D9_MAX_LIGHTS] = {};

    // Clip planes (D3D9 spec minimum = 6, 4 floats each)
    static constexpr UINT      SW_D3D9_MAX_CLIP_PLANES = 6;
    Float                      _clipPlanes[SW_D3D9_MAX_CLIP_PLANES][4] = {};
    D3DCLIPSTATUS9             _clipStatus          = {};

    // Fixed-function shader cache: synthesized + JIT-compiled shaders
    struct FFCacheEntry { void* fn; D3DSW_ParsedShader refl; };
    mutable std::map<FFVSKey, FFCacheEntry> _ffVsCache;
    mutable std::map<FFPSKey, FFCacheEntry> _ffPsCache;

    D3D9StateBlockSW*  _pendingStateBlock = nullptr;

private:
    FFCacheEntry* GetFFVS() const;
    FFCacheEntry* GetFFPS() const;
};

}
