#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include "d3d9/device/device.h"
#include "d3d9/factory/direct3d9.h"
#include "d3d9/resources/cube_texture.h"
#include "d3d9/resources/surface.h"
#include "d3d9/resources/texture.h"
#include "d3d9/resources/volume_texture.h"
#include "d3d9/resources/vertex_buffer.h"
#include "d3d9/resources/index_buffer.h"
#include "d3d9/shaders/pixel_shader.h"
#include "d3d9/shaders/vertex_declaration.h"
#include "d3d9/shaders/vertex_shader.h"
#include "d3d9/swapchain/swap_chain.h"
#include "d3d9/translate/d3dformat_bridge.h"
#include "d3d9/context/pipeline_state_builder.h"
#include "d3d9/state_block/state_block.h"
#include "d3d9/misc/query.h"
#include "core/common/trace.h"
#include "core/rasterizer/depth_stencil_util.h"
#include "core/format/pack_util.h"
#include "core/pipeline/sw_pipeline_state.h"
#include "core/shaders/shader_jit.h"
#include "core/util/format.h"


namespace d3dsw {

D3D9DeviceSW::D3D9DeviceSW(D3D9SW* parent, const D3DPRESENT_PARAMETERS& params, HWND focusWindow, Bool isEx)
    : _parent(parent), _isEx(isEx), _focusWindow(focusWindow)
{
    if (_parent) 
    { 
        _parent->AddRef(); 
    }

    _rs[D3DRS_ZENABLE]              = D3DZB_TRUE;
    _rs[D3DRS_FILLMODE]             = D3DFILL_SOLID;
    _rs[D3DRS_ALPHATESTENABLE]      = FALSE;
    _rs[D3DRS_ALPHAREF]             = 0;
    _rs[D3DRS_ALPHAFUNC]            = D3DCMP_ALWAYS;
    _rs[D3DRS_ZWRITEENABLE]         = TRUE;
    _rs[D3DRS_SRCBLEND]             = D3DBLEND_ONE;
    _rs[D3DRS_DESTBLEND]            = D3DBLEND_ZERO;
    _rs[D3DRS_CULLMODE]             = D3DCULL_CCW;
    _rs[D3DRS_ZFUNC]                = D3DCMP_LESSEQUAL;
    _rs[D3DRS_ALPHABLENDENABLE]     = FALSE;
    _rs[D3DRS_STENCILFAIL]          = D3DSTENCILOP_KEEP;
    _rs[D3DRS_STENCILZFAIL]         = D3DSTENCILOP_KEEP;
    _rs[D3DRS_STENCILPASS]          = D3DSTENCILOP_KEEP;
    _rs[D3DRS_STENCILFUNC]          = D3DCMP_ALWAYS;
    _rs[D3DRS_STENCILENABLE]        = FALSE;
    _rs[D3DRS_STENCILREF]           = 0;
    _rs[D3DRS_STENCILMASK]          = 0xFFFFFFFFu;
    _rs[D3DRS_STENCILWRITEMASK]     = 0xFFFFFFFFu;
    _rs[D3DRS_COLORWRITEENABLE]     = 0xFu;
    _rs[D3DRS_BLENDOP]              = D3DBLENDOP_ADD;
    _rs[D3DRS_SCISSORTESTENABLE]    = FALSE;
    _rs[D3DRS_SLOPESCALEDEPTHBIAS]  = 0;   // float 0.0 stored as DWORD
    _rs[D3DRS_TWOSIDEDSTENCILMODE]  = FALSE;
    _rs[D3DRS_CCW_STENCILFAIL]      = D3DSTENCILOP_KEEP;
    _rs[D3DRS_CCW_STENCILZFAIL]     = D3DSTENCILOP_KEEP;
    _rs[D3DRS_CCW_STENCILPASS]      = D3DSTENCILOP_KEEP;
    _rs[D3DRS_CCW_STENCILFUNC]      = D3DCMP_ALWAYS;
    _rs[D3DRS_COLORWRITEENABLE1]    = 0xFu;
    _rs[D3DRS_COLORWRITEENABLE2]    = 0xFu;
    _rs[D3DRS_COLORWRITEENABLE3]    = 0xFu;
    _rs[D3DRS_DEPTHBIAS]            = 0;   // float 0.0 stored as DWORD
    _rs[D3DRS_SRCBLENDALPHA]        = D3DBLEND_ONE;
    _rs[D3DRS_DESTBLENDALPHA]       = D3DBLEND_ZERO;
    _rs[D3DRS_BLENDOPALPHA]         = D3DBLENDOP_ADD;
    _rs[D3DRS_LIGHTING]             = TRUE;
    _rs[D3DRS_SPECULARENABLE]       = FALSE;
    _rs[D3DRS_AMBIENT]              = 0;
    _rs[D3DRS_NORMALIZENORMALS]     = FALSE;
    _rs[D3DRS_COLORVERTEX]          = TRUE;
    _rs[D3DRS_DIFFUSEMATERIALSOURCE]  = D3DMCS_COLOR1;
    _rs[D3DRS_SPECULARMATERIALSOURCE] = D3DMCS_COLOR2;
    _rs[D3DRS_AMBIENTMATERIALSOURCE]  = D3DMCS_MATERIAL;
    _rs[D3DRS_EMISSIVEMATERIALSOURCE] = D3DMCS_MATERIAL;

    auto setIdentity = [](D3DMATRIX& m) 
    {
        m = {};
        m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.f;
    };
    setIdentity(_transforms[D3DTS_VIEW]);
    setIdentity(_transforms[D3DTS_PROJECTION]);
    for (Uint i = 0; i < 256; ++i) 
    { 
        setIdentity(_transforms[D3DTS_WORLDMATRIX(i)]); 
    }

    for (Uint s = 0; s < SW_D3D9_MAX_TEXTURE_STAGES; ++s)
    {
        _samplerStates[s][D3DSAMP_ADDRESSU]     = D3DTADDRESS_WRAP;
        _samplerStates[s][D3DSAMP_ADDRESSV]     = D3DTADDRESS_WRAP;
        _samplerStates[s][D3DSAMP_ADDRESSW]     = D3DTADDRESS_WRAP;
        _samplerStates[s][D3DSAMP_MAGFILTER]    = D3DTEXF_POINT;
        _samplerStates[s][D3DSAMP_MINFILTER]    = D3DTEXF_POINT;
        _samplerStates[s][D3DSAMP_MIPFILTER]    = D3DTEXF_NONE;
        _samplerStates[s][D3DSAMP_MAXANISOTROPY] = 1;
        _samplerStates[s][D3DSAMP_MAXMIPLEVEL]  = 0;
        _samplerStates[s][D3DSAMP_MIPMAPLODBIAS] = 0;
    }

    for (Uint s = 0; s < SW_D3D9_MAX_TEXTURE_STAGES; ++s)
    {
        _tss[s][D3DTSS_COLOROP]               = (s == 0) ? D3DTOP_MODULATE : D3DTOP_DISABLE;
        _tss[s][D3DTSS_COLORARG1]             = D3DTA_TEXTURE;
        _tss[s][D3DTSS_COLORARG2]             = D3DTA_CURRENT;
        _tss[s][D3DTSS_ALPHAOP]               = (s == 0) ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE;
        _tss[s][D3DTSS_ALPHAARG1]             = D3DTA_TEXTURE;
        _tss[s][D3DTSS_ALPHAARG2]             = D3DTA_CURRENT;
        _tss[s][D3DTSS_TEXCOORDINDEX]         = s;
        _tss[s][D3DTSS_TEXTURETRANSFORMFLAGS] = D3DTTFF_DISABLE;
        _tss[s][D3DTSS_COLORARG0]             = D3DTA_CURRENT;
        _tss[s][D3DTSS_ALPHAARG0]             = D3DTA_CURRENT;
        _tss[s][D3DTSS_RESULTARG]             = D3DTA_CURRENT;
    }

    _implicitSwapChain = new D3D9SwapChainSW(this, params, focusWindow);
    _renderTargets[0] = _implicitSwapChain->BackBuffer();
    if (_renderTargets[0])
    {
        _renderTargets[0]->AddRef();
        _viewport.X      = 0;
        _viewport.Y      = 0;
        _viewport.Width  = _renderTargets[0]->Width();
        _viewport.Height = _renderTargets[0]->Height();
        _viewport.MinZ   = 0.f;
        _viewport.MaxZ   = 1.f;
        _scissor.left    = 0;
        _scissor.top     = 0;
        _scissor.right   = static_cast<LONG>(_renderTargets[0]->Width());
        _scissor.bottom  = static_cast<LONG>(_renderTargets[0]->Height());
    }

    if (params.EnableAutoDepthStencil && _renderTargets[0])
    {
        DXGI_FORMAT dxgiFmt = D3DFormatToDXGI(params.AutoDepthStencilFormat);
        _currentDSV = new D3D9SurfaceSW(this,
            _renderTargets[0]->Width(), _renderTargets[0]->Height(),
            params.AutoDepthStencilFormat, dxgiFmt, D3DRTYPE_SURFACE);
    }
}

D3D9DeviceSW::~D3D9DeviceSW()
{
    for (auto& rt : _renderTargets)
    {
        if (rt) { rt->Release(); rt = nullptr; }
    }
    if (_currentDSV)  { _currentDSV->Release();  _currentDSV  = nullptr; }
    for (auto& s : _streams)
    {
        if (s.buffer) { s.buffer->Release(); s.buffer = nullptr; }
    }
    if (_indices)     { _indices->Release();     _indices     = nullptr; }
    if (_vertexDecl)  { _vertexDecl->Release();  _vertexDecl  = nullptr; }
    if (_vs)          { _vs->Release();          _vs          = nullptr; }
    if (_ps)          { _ps->Release();          _ps          = nullptr; }
    for (auto& tex : _textures)
    {
        if (tex) { tex->Release(); tex = nullptr; }
    }
    if (_implicitSwapChain) { _implicitSwapChain->Release(); }
    if (_parent)            { _parent->Release(); }
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) { return E_POINTER; }
    if (riid == IID_IUnknown || riid == IID_IDirect3DDevice9)
    {
        *ppv = static_cast<IDirect3DDevice9*>(this);
        AddRef();
        return S_OK;
    }
    if (_isEx && riid == IID_IDirect3DDevice9Ex)
    {
        *ppv = static_cast<IDirect3DDevice9Ex*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::TestCooperativeLevel() { return S_OK; }
UINT    STDMETHODCALLTYPE D3D9DeviceSW::GetAvailableTextureMem() { return 0; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::EvictManagedResources() { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetDirect3D(IDirect3D9** ppD3D9)
{
    if (!ppD3D9)
    {
        return D3DERR_INVALIDCALL;
    }
    *ppD3D9 = static_cast<IDirect3D9*>(_parent);
    if (_parent)
    {
        _parent->AddRef();
    }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetDeviceCaps(D3DCAPS9* pCaps) { return _parent->GetDeviceCaps(0, D3DDEVTYPE_HAL, pCaps); }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode)
{
    if (iSwapChain != 0 || !pMode) { return D3DERR_INVALIDCALL; }
    pMode->Width       = _renderTargets[0] ? _renderTargets[0]->Width()  : 1920;
    pMode->Height      = _renderTargets[0] ? _renderTargets[0]->Height() : 1080;
    pMode->RefreshRate = 60;
    pMode->Format      = D3DFMT_X8R8G8B8;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters)
{
    if (!pParameters) { return D3DERR_INVALIDCALL; }
    pParameters->AdapterOrdinal     = D3DADAPTER_DEFAULT;
    pParameters->DeviceType         = D3DDEVTYPE_HAL;
    pParameters->hFocusWindow       = nullptr;
    pParameters->BehaviorFlags      = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetCursorProperties(UINT, UINT, IDirect3DSurface9*) { return D3DERR_NOTAVAILABLE; }
void    STDMETHODCALLTYPE D3D9DeviceSW::SetCursorPosition(int, int, DWORD) {}
BOOL    STDMETHODCALLTYPE D3D9DeviceSW::ShowCursor(BOOL) { return FALSE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS*, IDirect3DSwapChain9**) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain)
{
    if (!pSwapChain)
    {
        return D3DERR_INVALIDCALL;
    }
    if (iSwapChain != 0 || !_implicitSwapChain)
    {
        *pSwapChain = nullptr;
        return D3DERR_INVALIDCALL;
    }
    _implicitSwapChain->AddRef();
    *pSwapChain = _implicitSwapChain;
    return S_OK;
}
UINT    STDMETHODCALLTYPE D3D9DeviceSW::GetNumberOfSwapChains() { return _implicitSwapChain ? 1 : 0; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    if (!pPresentationParameters) 
    { 
        return D3DERR_INVALIDCALL; 
    }

    for (auto& rt : _renderTargets)
    {
        if (rt) 
        { 
            rt->Release(); 
            rt = nullptr; 
        }
    }
    if (_currentDSV) 
    { 
        _currentDSV->Release(); 
        _currentDSV = nullptr;
    }
    if (_implicitSwapChain) 
    { 
        _implicitSwapChain->Release(); 
        _implicitSwapChain = nullptr; 
    }

    _implicitSwapChain = new D3D9SwapChainSW(this, *pPresentationParameters, _focusWindow);
    _implicitSwapChain->GetPresentParameters(pPresentationParameters);
    _renderTargets[0] = _implicitSwapChain->BackBuffer();
    if (_renderTargets[0])
    {
        _renderTargets[0]->AddRef();
        _viewport.X      = 0;
        _viewport.Y      = 0;
        _viewport.Width  = _renderTargets[0]->Width();
        _viewport.Height = _renderTargets[0]->Height();
        _viewport.MinZ   = 0.f;
        _viewport.MaxZ   = 1.f;
        _scissor.left    = 0;
        _scissor.top     = 0;
        _scissor.right   = static_cast<LONG>(_renderTargets[0]->Width());
        _scissor.bottom  = static_cast<LONG>(_renderTargets[0]->Height());
    }

    if (pPresentationParameters->EnableAutoDepthStencil && _renderTargets[0])
    {
        DXGI_FORMAT dxgiFmt = D3DFormatToDXGI(pPresentationParameters->AutoDepthStencilFormat);
        _currentDSV = new D3D9SurfaceSW(this,
            _renderTargets[0]->Width(), _renderTargets[0]->Height(),
            pPresentationParameters->AutoDepthStencilFormat, dxgiFmt, D3DRTYPE_SURFACE);
    }

    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::Present(const RECT* src, const RECT* dst, HWND wnd, const RGNDATA* dirty)
{
    D3DSW_TRACE_PRESENT("Present");
    if (!_implicitSwapChain)
    {
        return D3DERR_INVALIDCALL;
    }
    return _implicitSwapChain->Present(src, dst, wnd, dirty, 0);
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer)
{
    if (iSwapChain != 0 || !_implicitSwapChain)
    {
        if (ppBackBuffer) { *ppBackBuffer = nullptr; }
        return D3DERR_INVALIDCALL;
    }
    return _implicitSwapChain->GetBackBuffer(iBackBuffer, Type, ppBackBuffer);
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus)
{
    if (!pRasterStatus) { return D3DERR_INVALIDCALL; }
    pRasterStatus->InVBlank = TRUE;
    pRasterStatus->ScanLine = 0;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetDialogBoxMode(BOOL) { return D3DERR_NOTAVAILABLE; }
void    STDMETHODCALLTYPE D3D9DeviceSW::SetGammaRamp(UINT, DWORD, const D3DGAMMARAMP*) {}
void    STDMETHODCALLTYPE D3D9DeviceSW::GetGammaRamp(UINT, D3DGAMMARAMP*) {}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage,
                                                      D3DFORMAT Format, D3DPOOL Pool,
                                                      IDirect3DTexture9** ppTexture, HANDLE* /*pSharedHandle*/)
{
    D3DSW_TRACE_CREATE("CreateTexture", "{}x{}, Levels={}, Usage=0x{:X}, Format={}", Width, Height, Levels, Usage, static_cast<Uint32>(Format));
    if (!ppTexture || Width == 0 || Height == 0)
    {
        if (ppTexture) { *ppTexture = nullptr; }
        return D3DERR_INVALIDCALL;
    }
    DXGI_FORMAT dxgiFmt = D3DFormatToDXGI(Format);
    if (dxgiFmt == DXGI_FORMAT_UNKNOWN)
    {
        *ppTexture = nullptr;
        return D3DERR_INVALIDCALL;
    }
    *ppTexture = new D3D9TextureSW(this, Width, Height, Levels, Usage, Format, dxgiFmt, Pool);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateVolumeTexture(UINT Width, UINT Height, UINT Depth,
                                                            UINT Levels, DWORD Usage, D3DFORMAT Format,
                                                            D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture,
                                                            HANDLE* /*pSharedHandle*/)
{
    D3DSW_TRACE_CREATE("CreateVolumeTexture", "{}x{}x{}, Levels={}, Format={}", Width, Height, Depth, Levels, static_cast<Uint32>(Format));
    if (!ppVolumeTexture || Width == 0 || Height == 0 || Depth == 0) { return D3DERR_INVALIDCALL; }
    DXGI_FORMAT dxgi = D3DFormatToDXGI(Format);
    if (dxgi == DXGI_FORMAT_UNKNOWN) { return D3DERR_INVALIDCALL; }
    *ppVolumeTexture = new D3D9VolumeTextureSW(this, Width, Height, Depth, Levels, Usage, Format, dxgi, Pool);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage,
                                                            D3DFORMAT Format, D3DPOOL Pool,
                                                            IDirect3DCubeTexture9** ppCubeTexture,
                                                            HANDLE* pSharedHandle)
{
    D3DSW_TRACE_CREATE("CreateCubeTexture", "EdgeLength={}, Levels={}, Format={}", EdgeLength, Levels, static_cast<Uint32>(Format));
    if (!ppCubeTexture || EdgeLength == 0) { return D3DERR_INVALIDCALL; }
    DXGI_FORMAT dxgi = D3DFormatToDXGI(Format);
    if (dxgi == DXGI_FORMAT_UNKNOWN) { return D3DERR_INVALIDCALL; }
    *ppCubeTexture = new D3D9CubeTextureSW(this, EdgeLength, Levels, Usage, Format, dxgi, Pool);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool,
                                                           IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* /*pSharedHandle*/)
{
    D3DSW_TRACE_CREATE("CreateVertexBuffer", "Length={}, Usage=0x{:X}, FVF=0x{:X}", Length, Usage, FVF);
    if (!ppVertexBuffer || Length == 0)
    {
        if (ppVertexBuffer) { *ppVertexBuffer = nullptr; }
        return D3DERR_INVALIDCALL;
    }
    *ppVertexBuffer = new D3D9VertexBufferSW(this, Length, Usage, FVF, Pool);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                                                          IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* /*pSharedHandle*/)
{
    D3DSW_TRACE_CREATE("CreateIndexBuffer", "Length={}, Usage=0x{:X}, Format={}", Length, Usage, static_cast<Uint32>(Format));
    if (!ppIndexBuffer || Length == 0)
    {
        if (ppIndexBuffer) { *ppIndexBuffer = nullptr; }
        return D3DERR_INVALIDCALL;
    }
    *ppIndexBuffer = new D3D9IndexBufferSW(this, Length, Usage, Format, Pool);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format,
                                                           D3DMULTISAMPLE_TYPE /*MultiSample*/, DWORD /*MultisampleQuality*/,
                                                           BOOL /*Lockable*/, IDirect3DSurface9** ppSurface, HANDLE* /*pSharedHandle*/)
{
    D3DSW_TRACE_CREATE("CreateRenderTarget", "{}x{}, Format={}", Width, Height, static_cast<Uint32>(Format));
    if (!ppSurface || Width == 0 || Height == 0)
    {
        if (ppSurface) { *ppSurface = nullptr; }
        return D3DERR_INVALIDCALL;
    }
    DXGI_FORMAT dxgiFmt = D3DFormatToDXGI(Format);
    if (dxgiFmt == DXGI_FORMAT_UNKNOWN)
    {
        *ppSurface = nullptr;
        return D3DERR_INVALIDCALL;
    }
    *ppSurface = new D3D9SurfaceSW(this, Width, Height, Format, dxgiFmt, D3DRTYPE_SURFACE);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format,
                                                                  D3DMULTISAMPLE_TYPE /*MultiSample*/, DWORD /*MultisampleQuality*/,
                                                                  BOOL /*Discard*/, IDirect3DSurface9** ppSurface, HANDLE* /*pSharedHandle*/)
{
    D3DSW_TRACE_CREATE("CreateDepthStencilSurface", "{}x{}, Format={}", Width, Height, static_cast<Uint32>(Format));
    if (!ppSurface || Width == 0 || Height == 0)
    {
        if (ppSurface) { *ppSurface = nullptr; }
        return D3DERR_INVALIDCALL;
    }
    DXGI_FORMAT dxgiFmt = D3DFormatToDXGI(Format);
    if (dxgiFmt == DXGI_FORMAT_UNKNOWN)
    {
        *ppSurface = nullptr;
        return D3DERR_INVALIDCALL;
    }
    *ppSurface = new D3D9SurfaceSW(this, Width, Height, Format, dxgiFmt, D3DRTYPE_SURFACE);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::UpdateSurface(IDirect3DSurface9* pSourceSurface, const RECT* pSourceRect,
                                                      IDirect3DSurface9* pDestSurface,   const POINT* pDestPoint)
{
    if (!pSourceSurface || !pDestSurface) { return D3DERR_INVALIDCALL; }
    D3D9SurfaceSW* src = static_cast<D3D9SurfaceSW*>(pSourceSurface);
    D3D9SurfaceSW* dst = static_cast<D3D9SurfaceSW*>(pDestSurface);
    if (src->D3DFormat() != dst->D3DFormat())  { return D3DERR_INVALIDCALL; }
    if (src->PixStride() != dst->PixStride())  { return D3DERR_INVALIDCALL; }

    Uint srcX = pSourceRect ? static_cast<Uint>(pSourceRect->left)  : 0;
    Uint srcY = pSourceRect ? static_cast<Uint>(pSourceRect->top)   : 0;
    Uint copyW = pSourceRect ? static_cast<Uint>(pSourceRect->right  - pSourceRect->left) : src->Width();
    Uint copyH = pSourceRect ? static_cast<Uint>(pSourceRect->bottom - pSourceRect->top)  : src->Height();
    Uint dstX = pDestPoint ? static_cast<Uint>(pDestPoint->x) : 0;
    Uint dstY = pDestPoint ? static_cast<Uint>(pDestPoint->y) : 0;

    copyW = std::min(copyW, dst->Width()  - dstX);
    copyH = std::min(copyH, dst->Height() - dstY);
    Uint stride  = src->PixStride();
    Uint rowBytes = copyW * stride;
    for (Uint y = 0; y < copyH; ++y)
    {
        const Uint8* srcRow = src->DataPtr() + (srcY + y) * src->RowPitch() + srcX * stride;
        Uint8*       dstRow = dst->DataPtr() + (dstY + y) * dst->RowPitch() + dstX * stride;
        std::memcpy(dstRow, srcRow, rowBytes);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestTexture)
{
    if (!pSourceTexture || !pDestTexture) { return D3DERR_INVALIDCALL; }
    if (pSourceTexture->GetType() != pDestTexture->GetType()) { return D3DERR_INVALIDCALL; }

    if (pSourceTexture->GetType() == D3DRTYPE_TEXTURE)
    {
        D3D9TextureSW* src = static_cast<D3D9TextureSW*>(pSourceTexture);
        D3D9TextureSW* dst = static_cast<D3D9TextureSW*>(pDestTexture);
        DWORD levels = std::min(src->GetLevelCount(), dst->GetLevelCount());
        for (DWORD lvl = 0; lvl < levels; ++lvl)
        {
            IDirect3DSurface9* srcSurf = nullptr;
            IDirect3DSurface9* dstSurf = nullptr;
            src->GetSurfaceLevel(lvl, &srcSurf);
            dst->GetSurfaceLevel(lvl, &dstSurf);
            if (srcSurf && dstSurf)
            {
                UpdateSurface(srcSurf, nullptr, dstSurf, nullptr);
            }
            if (srcSurf) { srcSurf->Release(); }
            if (dstSurf) { dstSurf->Release(); }
        }
        return S_OK;
    }
    return D3DERR_INVALIDCALL;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetRenderTargetData(IDirect3DSurface9*, IDirect3DSurface9*) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetFrontBufferData(UINT, IDirect3DSurface9*) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::StretchRect(IDirect3DSurface9*, const RECT*, IDirect3DSurface9*, const RECT*, D3DTEXTUREFILTERTYPE) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::ColorFill(IDirect3DSurface9*, const RECT*, D3DCOLOR) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format,
                                                                    D3DPOOL /*Pool*/, IDirect3DSurface9** ppSurface,
                                                                    HANDLE* /*pSharedHandle*/)
{
    D3DSW_TRACE_CREATE("CreateOffscreenPlainSurface", "{}x{}, Format={}", Width, Height, static_cast<Uint32>(Format));
    if (!ppSurface || Width == 0 || Height == 0)
    {
        if (ppSurface) { *ppSurface = nullptr; }
        return D3DERR_INVALIDCALL;
    }
    DXGI_FORMAT dxgiFmt = D3DFormatToDXGI(Format);
    if (dxgiFmt == DXGI_FORMAT_UNKNOWN)
    {
        *ppSurface = nullptr;
        return D3DERR_INVALIDCALL;
    }
    *ppSurface = new D3D9SurfaceSW(this, Width, Height, Format, dxgiFmt, D3DRTYPE_SURFACE);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget)
{
    D3DSW_TRACE_STATE("SetRenderTarget", "Index={}, pSurface={}", RenderTargetIndex, static_cast<void*>(pRenderTarget));
    if (RenderTargetIndex >= SW_D3D9_MAX_RTS)
    {
        return D3DERR_INVALIDCALL;
    }
    if (RenderTargetIndex == 0 && !pRenderTarget)
    {
        return D3DERR_INVALIDCALL;  
    }
    D3D9SurfaceSW* surf = static_cast<D3D9SurfaceSW*>(pRenderTarget);
    if (surf)
    {
        surf->AddRef();
    }
    if (_renderTargets[RenderTargetIndex])
    {
        _renderTargets[RenderTargetIndex]->Release();
    }
    _renderTargets[RenderTargetIndex] = surf;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget)
{
    if (!ppRenderTarget)
    {
        return D3DERR_INVALIDCALL;
    }
    if (RenderTargetIndex >= SW_D3D9_MAX_RTS || !_renderTargets[RenderTargetIndex])
    {
        *ppRenderTarget = nullptr;
        return D3DERR_NOTFOUND;
    }
    _renderTargets[RenderTargetIndex]->AddRef();
    *ppRenderTarget = _renderTargets[RenderTargetIndex];
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil)
{
    D3DSW_TRACE_STATE("SetDepthStencilSurface", "pSurface={}", static_cast<void*>(pNewZStencil));
    D3D9SurfaceSW* surf = static_cast<D3D9SurfaceSW*>(pNewZStencil);
    if (surf)
    {
        surf->AddRef();
    }
    if (_currentDSV)
    {
        _currentDSV->Release();
    }
    _currentDSV = surf;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface)
{
    if (!ppZStencilSurface)
    {
        return D3DERR_INVALIDCALL;
    }
    *ppZStencilSurface = _currentDSV;
    if (_currentDSV)
    {
        _currentDSV->AddRef();
        return S_OK;
    }
    return D3DERR_NOTFOUND;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::BeginScene() { _inScene = true; return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::EndScene() { _inScene = false; return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::Clear(DWORD /*rect_count*/, const D3DRECT* /*rects*/, DWORD flags, D3DCOLOR color, float z, DWORD stencil)
{
    D3DSW_TRACE_DRAW("Clear", "flags=0x{:X}, color=0x{:08X}, z={}, stencil={}", flags, color, z, stencil);
    // D3DCOLOR is 0xAARRGGBB
    if ((flags & D3DCLEAR_TARGET) && _renderTargets[0])
    {
        D3D9SurfaceSW* rt = _renderTargets[0];
        Float rgba[4] = {
            ((color >> 16) & 0xFF) / 255.f, // R
            ((color >>  8) & 0xFF) / 255.f, // G
            ((color >>  0) & 0xFF) / 255.f, // B
            ((color >> 24) & 0xFF) / 255.f, // A
        };
        Uint8 packed[16] = {};
        PackColor(rt->DxgiFormat(), rgba, packed);
        Uint32 pixStride = rt->PixStride();
        Uint32 rowPitch  = rt->RowPitch();
        Uint32 w = rt->Width();
        Uint32 h = rt->Height();
        Uint8* data = rt->DataPtr();
        for (Uint32 y = 0; y < h; ++y)
        {
            Uint8* row = data + static_cast<Usize>(y) * rowPitch;
            for (Uint32 x = 0; x < w; ++x)
            {
                std::memcpy(row + x * pixStride, packed, pixStride);
            }
        }
    }

    const Bool clearZ   = (flags & D3DCLEAR_ZBUFFER)  && _currentDSV;
    const Bool clearSt  = (flags & D3DCLEAR_STENCIL)   && _currentDSV;
    if (clearZ || clearSt)
    {
        D3D9SurfaceSW* dsv    = _currentDSV;
        DXGI_FORMAT    fmt    = dsv->DxgiFormat();
        Uint32         stride = dsv->PixStride();
        Uint32         pitch  = dsv->RowPitch();
        Uint32         w      = dsv->Width();
        Uint32         h      = dsv->Height();
        Uint8*         data   = dsv->DataPtr();
        for (Uint32 y = 0; y < h; ++y)
        {
            Uint8* row = data + static_cast<Usize>(y) * pitch;
            for (Uint32 x = 0; x < w; ++x)
            {
                Uint8* px = row + x * stride;
                Uint32 pix = 0;
                std::memcpy(&pix, px, sizeof(Uint32));
                if (clearZ)  { pix = ClearDepthBits(fmt, pix, z); }
                if (clearSt) { pix = ClearStencilBits(fmt, pix, static_cast<Uint8>(stencil & 0xFF)); }
                std::memcpy(px, &pix, sizeof(Uint32));
            }
        }
    }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix)
{
    D3DSW_TRACE_STATE("SetTransform", "state={}", static_cast<Uint32>(state));
    if (!matrix || static_cast<UINT>(state) >= 512) { return D3DERR_INVALIDCALL; }
    _transforms[state] = *matrix;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX* pMatrix)
{
    if (!pMatrix || static_cast<UINT>(state) >= 512) { return D3DERR_INVALIDCALL; }
    *pMatrix = _transforms[state];
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::MultiplyTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix)
{
    if (!matrix || static_cast<UINT>(state) >= 512) { return D3DERR_INVALIDCALL; }
    const D3DMATRIX& a = _transforms[state];
    const D3DMATRIX& b = *matrix;
    D3DMATRIX c{};
    for (Int r = 0; r < 4; ++r)
    {
        for (Int col = 0; col < 4; ++col)
        {
            for (Int k = 0; k < 4; ++k)
            {
                c.m[r][col] += a.m[r][k] * b.m[k][col];
            }
        }
    }
    _transforms[state] = c;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetViewport(const D3DVIEWPORT9* viewport)
{
    D3DSW_TRACE_STATE("SetViewport", "X={}, Y={}, W={}, H={}", viewport ? viewport->X : 0, viewport ? viewport->Y : 0, viewport ? viewport->Width : 0, viewport ? viewport->Height : 0);
    if (!viewport)
    {
        return D3DERR_INVALIDCALL;
    }
    _viewport = *viewport;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetViewport(D3DVIEWPORT9* pViewport)
{
    if (!pViewport)
    {
        return D3DERR_INVALIDCALL;
    }
    *pViewport = _viewport;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetMaterial(const D3DMATERIAL9* material)
{
    D3DSW_TRACE_STATE("SetMaterial");
    if (!material) { return D3DERR_INVALIDCALL; }
    _material = *material;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetMaterial(D3DMATERIAL9* pMaterial)
{
    if (!pMaterial) { return D3DERR_INVALIDCALL; }
    *pMaterial = _material;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetLight(DWORD index, const D3DLIGHT9* light)
{
    D3DSW_TRACE_STATE("SetLight", "index={}", index);
    if (!light || index >= SW_D3D9_MAX_LIGHTS) { return D3DERR_INVALIDCALL; }
    _lights[index] = *light;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetLight(DWORD Index, D3DLIGHT9* pLight)
{
    if (!pLight || Index >= SW_D3D9_MAX_LIGHTS) { return D3DERR_INVALIDCALL; }
    *pLight = _lights[Index];
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::LightEnable(DWORD Index, BOOL Enable)
{
    D3DSW_TRACE_STATE("LightEnable", "Index={}, Enable={}", Index, Enable);
    if (Index >= SW_D3D9_MAX_LIGHTS) { return D3DERR_INVALIDCALL; }
    _lightEnabled[Index] = Enable;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetLightEnable(DWORD Index, BOOL* pEnable)
{
    if (!pEnable || Index >= SW_D3D9_MAX_LIGHTS) { return D3DERR_INVALIDCALL; }
    *pEnable = _lightEnabled[Index];
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetClipPlane(DWORD index, const float* plane)
{
    if (!plane || index >= SW_D3D9_MAX_CLIP_PLANES) { return D3DERR_INVALIDCALL; }
    std::memcpy(_clipPlanes[index], plane, 4 * sizeof(Float));
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetClipPlane(DWORD Index, float* pPlane)
{
    if (!pPlane || Index >= SW_D3D9_MAX_CLIP_PLANES) { return D3DERR_INVALIDCALL; }
    std::memcpy(pPlane, _clipPlanes[Index], 4 * sizeof(Float));
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
    D3DSW_TRACE_STATE("SetRenderState", "State={}, Value={}", static_cast<Uint32>(State), Value);
    if (static_cast<UINT>(State) >= std::size(_rs)) { return D3DERR_INVALIDCALL; }
    _rs[State] = Value;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue)
{
    if (!pValue || static_cast<UINT>(State) >= std::size(_rs)) { return D3DERR_INVALIDCALL; }
    *pValue = _rs[State];
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB)
{
    if (!ppSB) { return D3DERR_INVALIDCALL; }
    D3D9StateBlockSW* sb = new D3D9StateBlockSW(this);
    sb->Capture(this);
    *ppSB = sb;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::BeginStateBlock()
{
    if (_pendingStateBlock) { return D3DERR_INVALIDCALL; }
    _pendingStateBlock = new D3D9StateBlockSW(this);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::EndStateBlock(IDirect3DStateBlock9** ppSB)
{
    if (!ppSB || !_pendingStateBlock) { return D3DERR_INVALIDCALL; }
    _pendingStateBlock->Capture(this);
    *ppSB = _pendingStateBlock;
    _pendingStateBlock = nullptr;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetClipStatus(const D3DCLIPSTATUS9* clip_status)
{
    if (!clip_status) { return D3DERR_INVALIDCALL; }
    _clipStatus = *clip_status;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetClipStatus(D3DCLIPSTATUS9* pClipStatus)
{
    if (!pClipStatus) { return D3DERR_INVALIDCALL; }
    *pClipStatus = _clipStatus;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture)
{
    if (!ppTexture || Stage >= SW_D3D9_MAX_TEXTURE_STAGES) { return D3DERR_INVALIDCALL; }
    *ppTexture = _textures[Stage];
    if (_textures[Stage]) { _textures[Stage]->AddRef(); }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture)
{
    D3DSW_TRACE_STATE("SetTexture", "Stage={}, pTexture={}", Stage, static_cast<void*>(pTexture));
    if (Stage >= SW_D3D9_MAX_TEXTURE_STAGES) { return D3DERR_INVALIDCALL; }
    if (pTexture) { pTexture->AddRef(); }
    if (_textures[Stage]) { _textures[Stage]->Release(); }
    _textures[Stage] = pTexture;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)
{
    if (!pValue || Stage >= SW_D3D9_MAX_TEXTURE_STAGES ||
        Type < 1 || Type > D3DTSS_CONSTANT) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    *pValue = _tss[Stage][Type];
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    D3DSW_TRACE_STATE("SetTextureStageState", "Stage={}, Type={}, Value={}", Stage, static_cast<Uint32>(Type), Value);
    if (Stage >= SW_D3D9_MAX_TEXTURE_STAGES ||
        Type < 1 || Type > D3DTSS_CONSTANT) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    _tss[Stage][Type] = Value;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue)
{
    if (!pValue || Sampler >= SW_D3D9_MAX_TEXTURE_STAGES ||
        Type < 1 || Type > D3DSAMP_DMAPOFFSET) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    *pValue = _samplerStates[Sampler][Type];
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    D3DSW_TRACE_STATE("SetSamplerState", "Sampler={}, Type={}, Value={}", Sampler, static_cast<Uint32>(Type), Value);
    if (Sampler >= SW_D3D9_MAX_TEXTURE_STAGES ||
        Type < 1 || Type > D3DSAMP_DMAPOFFSET) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    _samplerStates[Sampler][Type] = Value;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::ValidateDevice(DWORD* pNumPasses)
{
    if (pNumPasses) { *pNumPasses = 1; }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetPaletteEntries(UINT, const PALETTEENTRY*) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetPaletteEntries(UINT, PALETTEENTRY*) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetCurrentTexturePalette(UINT) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetCurrentTexturePalette(UINT*) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetScissorRect(const RECT* rect)
{
    D3DSW_TRACE_STATE("SetScissorRect", "L={}, T={}, R={}, B={}", rect ? rect->left : 0, rect ? rect->top : 0, rect ? rect->right : 0, rect ? rect->bottom : 0);
    if (!rect)
    {
        return D3DERR_INVALIDCALL;
    }
    _scissor = *rect;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetScissorRect(RECT* pRect)
{
    if (!pRect)
    {
        return D3DERR_INVALIDCALL;
    }
    *pRect = _scissor;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetSoftwareVertexProcessing(BOOL) { return D3DERR_NOTAVAILABLE; }
BOOL STDMETHODCALLTYPE D3D9DeviceSW::GetSoftwareVertexProcessing() { return FALSE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetNPatchMode(float) { return D3DERR_NOTAVAILABLE; }
float   STDMETHODCALLTYPE D3D9DeviceSW::GetNPatchMode() { return 0.f; }

namespace {
// D3D9 expresses draws as a primitive count + topology
Uint VerticesForPrimitives(D3DPRIMITIVETYPE pt, Uint primCount)
{
    switch (pt)
    {
        case D3DPT_POINTLIST:     return primCount;
        case D3DPT_LINELIST:      return primCount * 2;
        case D3DPT_LINESTRIP:     return primCount + 1;
        case D3DPT_TRIANGLELIST:  return primCount * 3;
        case D3DPT_TRIANGLESTRIP: return primCount + 2;
        case D3DPT_TRIANGLEFAN:   return primCount + 2;  
        default:                  return 0;
    }
}
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    D3DSW_TRACE_DRAW("DrawPrimitive", "PrimitiveType={}, StartVertex={}, PrimitiveCount={}", static_cast<Uint32>(PrimitiveType), StartVertex, PrimitiveCount);
    Uint vertexCount = VerticesForPrimitives(PrimitiveType, PrimitiveCount);
    if (vertexCount == 0)
    {
        return D3DERR_INVALIDCALL;
    }

    D3D9SW_DRAW_STATE snap{};
    SnapshotDrawState(snap, PrimitiveType);
    SWPipelineState swState{};
    BuildSWPipelineState(snap, swState);
    if (swState.topology == SWTopology::Undefined)
    {
        return D3DERR_INVALIDCALL;
    }

    SWDrawCommand cmd{};
    cmd.vertexCount   = vertexCount;
    cmd.startVertex   = StartVertex;
    cmd.instanceCount = (_streamFreq[0] & D3DSTREAMSOURCE_INDEXEDDATA)
                        ? std::max(1u, _streamFreq[0] & 0x3FFFFFFFu)
                        : 1u;
    _rasterizer.Draw(cmd, swState);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT /*MinVertexIndex*/, UINT /*NumVertices*/, UINT startIndex, UINT primCount)
{
    D3DSW_TRACE_DRAW("DrawIndexedPrimitive", "PrimitiveType={}, BaseVertexIndex={}, startIndex={}, primCount={}", static_cast<Uint32>(PrimitiveType), BaseVertexIndex, startIndex, primCount);
    Uint indexCount = VerticesForPrimitives(PrimitiveType, primCount);
    if (indexCount == 0)
    {
        return D3DERR_INVALIDCALL;
    }
    if (!_indices)
    {
        return D3DERR_INVALIDCALL;
    }

    D3D9SW_DRAW_STATE snap{};
    SnapshotDrawState(snap, PrimitiveType);
    SWPipelineState swState{};
    BuildSWPipelineState(snap, swState);
    if (swState.topology == SWTopology::Undefined)
    {
        return D3DERR_INVALIDCALL;
    }

    SWDrawCommand cmd{};
    cmd.indexCount    = indexCount;
    cmd.startIndex    = startIndex;
    cmd.baseVertex    = BaseVertexIndex;
    cmd.instanceCount = (_streamFreq[0] & D3DSTREAMSOURCE_INDEXEDDATA)
                        ? std::max(1u, _streamFreq[0] & 0x3FFFFFFFu)
                        : 1u;
    cmd.indexed       = true;
    _rasterizer.DrawIndexed(cmd, swState);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::DrawPrimitiveUP(D3DPRIMITIVETYPE primitive_type, UINT primitive_count,
                                                        const void* data, UINT stride)
{
    D3DSW_TRACE_DRAW("DrawPrimitiveUP", "PrimitiveType={}, PrimitiveCount={}, stride={}", static_cast<Uint32>(primitive_type), primitive_count, stride);
    if (!data || primitive_count == 0 || stride == 0) { return D3DERR_INVALIDCALL; }
    Uint vertexCount = VerticesForPrimitives(primitive_type, primitive_count);
    if (vertexCount == 0) { return D3DERR_INVALIDCALL; }

    Uint byteSize = vertexCount * stride;

    IDirect3DVertexBuffer9* scratch = nullptr;
    HRESULT hr = CreateVertexBuffer(byteSize, 0, 0, D3DPOOL_DEFAULT, &scratch, nullptr);
    if (FAILED(hr)) 
    {
        return hr;
    }
    void* mapped = nullptr;
    scratch->Lock(0, byteSize, &mapped, 0);
    std::memcpy(mapped, data, byteSize);
    scratch->Unlock();

    IDirect3DVertexBuffer9* prevVB  = nullptr;
    UINT                    prevOff = 0, prevStr = 0;
    GetStreamSource(0, &prevVB, &prevOff, &prevStr);
    SetStreamSource(0, scratch, 0, stride);
    hr = DrawPrimitive(primitive_type, 0, primitive_count);
    SetStreamSource(0, prevVB, prevOff, prevStr);
    if (prevVB) { prevVB->Release(); }
    scratch->Release();
    return hr;
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE primitive_type,
                                                                UINT min_vertex_idx, UINT vertex_count,
                                                                UINT primitive_count,
                                                                const void* index_data, D3DFORMAT index_format,
                                                                const void* data, UINT stride)
{
    D3DSW_TRACE_DRAW("DrawIndexedPrimitiveUP", "PrimitiveType={}, PrimitiveCount={}, stride={}", static_cast<Uint32>(primitive_type), primitive_count, stride);
    if (!data || !index_data || primitive_count == 0 || stride == 0) 
    { 
        return D3DERR_INVALIDCALL;
    }
    Uint indexCount = VerticesForPrimitives(primitive_type, primitive_count);
    if (indexCount == 0) 
    { 
        return D3DERR_INVALIDCALL; 
    }

    const Uint ibStride  = (index_format == D3DFMT_INDEX32) ? 4 : 2;
    const Uint vbBytes   = vertex_count * stride;
    const Uint ibBytes   = indexCount   * ibStride;

    IDirect3DVertexBuffer9* scratchVB = nullptr;
    IDirect3DIndexBuffer9*  scratchIB = nullptr;
    HRESULT hr = CreateVertexBuffer(vbBytes, 0, 0, D3DPOOL_DEFAULT, &scratchVB, nullptr);
    if (FAILED(hr)) 
    { 
        return hr; 
    }

    hr = CreateIndexBuffer(ibBytes, 0, index_format, D3DPOOL_DEFAULT, &scratchIB, nullptr);
    if (FAILED(hr)) 
    { 
        scratchVB->Release(); 
        return hr; 
    }

    void* mv = nullptr; scratchVB->Lock(0, vbBytes, &mv, 0); std::memcpy(mv, data, vbBytes);       scratchVB->Unlock();
    void* mi = nullptr; scratchIB->Lock(0, ibBytes, &mi, 0); std::memcpy(mi, index_data, ibBytes); scratchIB->Unlock();

    IDirect3DVertexBuffer9* prevVB  = nullptr;
    UINT                    prevOff = 0, prevStr = 0;
    GetStreamSource(0, &prevVB, &prevOff, &prevStr);
    IDirect3DIndexBuffer9* prevIB = nullptr;
    GetIndices(&prevIB);

    SetStreamSource(0, scratchVB, 0, stride);
    SetIndices(scratchIB);
    hr = DrawIndexedPrimitive(primitive_type, -static_cast<INT>(min_vertex_idx),
                              min_vertex_idx, vertex_count, 0, primitive_count);

    SetStreamSource(0, prevVB, prevOff, prevStr);
    SetIndices(prevIB);
    if (prevVB) { prevVB->Release(); }
    if (prevIB) { prevIB->Release(); }
    scratchVB->Release();
    scratchIB->Release();
    return hr;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::ProcessVertices(UINT, UINT, UINT, IDirect3DVertexBuffer9*, IDirect3DVertexDeclaration9*, DWORD) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateVertexDeclaration(const D3DVERTEXELEMENT9* elements, IDirect3DVertexDeclaration9** declaration)
{
    if (!elements || !declaration)
    {
        if (declaration) { *declaration = nullptr; }
        return D3DERR_INVALIDCALL;
    }
    *declaration = new D3D9VertexDeclarationSW(this, elements);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl)
{
    D3DSW_TRACE_STATE("SetVertexDeclaration", "pDecl={}", static_cast<void*>(pDecl));
    D3D9VertexDeclarationSW* decl = static_cast<D3D9VertexDeclarationSW*>(pDecl);
    if (decl) 
    { 
        decl->AddRef(); 
    }

    if (_vertexDecl) 
    { 
        _vertexDecl->Release(); 
    }
    _vertexDecl = decl;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl)
{
    if (!ppDecl)
    {
        return D3DERR_INVALIDCALL;
    }
    *ppDecl = _vertexDecl;
    if (_vertexDecl) 
    { 
        _vertexDecl->AddRef(); 
    }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetFVF(DWORD FVF) { D3DSW_TRACE_STATE("SetFVF", "FVF=0x{:X}", FVF); _fvf = FVF; return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetFVF(DWORD* pFVF)
{
    if (!pFVF)
    {
        return D3DERR_INVALIDCALL;
    }
    *pFVF = _fvf;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateVertexShader(const DWORD* byte_code, IDirect3DVertexShader9** shader)
{
    D3DSW_TRACE_CREATE("CreateVertexShader");
    if (!byte_code || !shader)
    {
        if (shader) 
        { 
            *shader = nullptr; 
        }
        return D3DERR_INVALIDCALL;
    }
    //D3D9 CreateVertexShader does not pass a length, the caller passes a
    //raw token stream terminated by D3DVS_END
    Usize lenTokens = 1;
    while (byte_code[lenTokens - 1] != D3DVS_END())
    {
        ++lenTokens;
    }
    D3D9VertexShaderSW* vs = new D3D9VertexShaderSW(this);
    HRESULT hr = vs->Init(byte_code, lenTokens * sizeof(DWORD));
    if (FAILED(hr))
    {
        vs->Release();
        *shader = nullptr;
        return hr;
    }
    *shader = vs;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetVertexShader(IDirect3DVertexShader9* pShader)
{
    D3DSW_TRACE_SHADER("SetVertexShader", "pShader={}", static_cast<void*>(pShader));
    D3D9VertexShaderSW* vs = static_cast<D3D9VertexShaderSW*>(pShader);
    if (vs) { vs->AddRef(); }
    if (_vs) { _vs->Release(); }
    _vs = vs;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetVertexShader(IDirect3DVertexShader9** ppShader)
{
    if (!ppShader)
    {
        return D3DERR_INVALIDCALL;
    }
    *ppShader = _vs;
    if (_vs) { _vs->AddRef(); }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetVertexShaderConstantF(UINT reg_idx, const float* data, UINT count)
{
    D3DSW_TRACE_STATE("SetVertexShaderConstantF", "reg={}, count={}", reg_idx, count);
    if (!data || reg_idx + count > 256)
    {
        return D3DERR_INVALIDCALL;
    }
    std::memcpy(&_vsConstF[reg_idx][0], data, count * 4 * sizeof(Float));
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    if (!pConstantData || StartRegister + Vector4fCount > D3DSW_ARRAYSIZE(_vsConstF))
    {
        return D3DERR_INVALIDCALL;
    }
    std::memcpy(pConstantData, &_vsConstF[StartRegister][0], Vector4fCount * 4 * sizeof(Float));
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetVertexShaderConstantI(UINT reg_idx, const int* data, UINT count)
{
    D3DSW_TRACE_STATE("SetVertexShaderConstantI", "reg={}, count={}", reg_idx, count);
    if (!data || reg_idx + count > D3DSW_ARRAYSIZE(_vsConstI))
    {
        return D3DERR_INVALIDCALL;
    }
    std::memcpy(&_vsConstI[reg_idx][0], data, count * 4 * sizeof(Int32));
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    if (!pConstantData || StartRegister + Vector4iCount > D3DSW_ARRAYSIZE(_vsConstI))
    {
        return D3DERR_INVALIDCALL;
    }
    std::memcpy(pConstantData, &_vsConstI[StartRegister][0], Vector4iCount * 4 * sizeof(Int32));
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetVertexShaderConstantB(UINT reg_idx, const BOOL* data, UINT count)
{
    D3DSW_TRACE_STATE("SetVertexShaderConstantB", "reg={}, count={}", reg_idx, count);
    if (!data || reg_idx + count > D3DSW_ARRAYSIZE(_vsConstB))
    {
        return D3DERR_INVALIDCALL;
    }
    for (Uint i = 0; i < count; ++i)
    {
        _vsConstB[reg_idx + i] = data[i] ? 1u : 0u;
    }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetVertexShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
    if (!pConstantData || StartRegister + BoolCount > D3DSW_ARRAYSIZE(_vsConstB))
    {
        return D3DERR_INVALIDCALL;
    }
    for (Uint i = 0; i < BoolCount; ++i)
    {
        pConstantData[i] = _vsConstB[StartRegister + i] ? TRUE : FALSE;
    }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride)
{
    D3DSW_TRACE_STATE("SetStreamSource", "Stream={}, pVB={}, Offset={}, Stride={}", StreamNumber, static_cast<void*>(pStreamData), OffsetInBytes, Stride);
    if (StreamNumber >= SW_D3D9_MAX_STREAMS)
    {
        return D3DERR_INVALIDCALL;
    }
    D3D9VertexBufferSW* vb = static_cast<D3D9VertexBufferSW*>(pStreamData);
    if (vb) { vb->AddRef(); }
    if (_streams[StreamNumber].buffer) { _streams[StreamNumber].buffer->Release(); }
    _streams[StreamNumber].buffer = vb;
    _streams[StreamNumber].offset = OffsetInBytes;
    _streams[StreamNumber].stride = Stride;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* OffsetInBytes, UINT* pStride)
{
    if (StreamNumber >= SW_D3D9_MAX_STREAMS)
    {
        return D3DERR_INVALIDCALL;
    }
    const auto& s = _streams[StreamNumber];
    if (ppStreamData)
    {
        *ppStreamData = s.buffer;
        if (s.buffer) { s.buffer->AddRef(); }
    }
    if (OffsetInBytes) { *OffsetInBytes = s.offset; }
    if (pStride)       { *pStride       = s.stride; }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetStreamSourceFreq(UINT StreamNumber, UINT FreqDivider)
{
    if (StreamNumber >= SW_D3D9_MAX_STREAMS) { return D3DERR_INVALIDCALL; }
    _streamFreq[StreamNumber] = FreqDivider;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetStreamSourceFreq(UINT StreamNumber, UINT* pDivider)
{
    if (StreamNumber >= SW_D3D9_MAX_STREAMS || !pDivider) { return D3DERR_INVALIDCALL; }
    *pDivider = _streamFreq[StreamNumber];
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetIndices(IDirect3DIndexBuffer9* pIndexData)
{
    D3DSW_TRACE_STATE("SetIndices", "pIndexData={}", static_cast<void*>(pIndexData));
    D3D9IndexBufferSW* ib = static_cast<D3D9IndexBufferSW*>(pIndexData);
    if (ib) { ib->AddRef(); }
    if (_indices) { _indices->Release(); }
    _indices = ib;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetIndices(IDirect3DIndexBuffer9** ppIndexData)
{
    if (!ppIndexData)
    {
        return D3DERR_INVALIDCALL;
    }
    *ppIndexData = _indices;
    if (_indices) { _indices->AddRef(); }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreatePixelShader(const DWORD* byte_code, IDirect3DPixelShader9** shader)
{
    D3DSW_TRACE_CREATE("CreatePixelShader");
    if (!byte_code || !shader)
    {
        if (shader) { *shader = nullptr; }
        return D3DERR_INVALIDCALL;
    }
    Usize lenTokens = 1;
    while (byte_code[lenTokens - 1] != 0x0000FFFFu)
    {
        ++lenTokens;
    }
    D3D9PixelShaderSW* ps = new D3D9PixelShaderSW(this);
    HRESULT hr = ps->Init(byte_code, lenTokens * sizeof(DWORD));
    if (FAILED(hr))
    {
        ps->Release();
        *shader = nullptr;
        return hr;
    }
    *shader = ps;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetPixelShader(IDirect3DPixelShader9* pShader)
{
    D3DSW_TRACE_SHADER("SetPixelShader", "pShader={}", static_cast<void*>(pShader));
    D3D9PixelShaderSW* ps = static_cast<D3D9PixelShaderSW*>(pShader);
    if (ps) { ps->AddRef(); }
    if (_ps) { _ps->Release(); }
    _ps = ps;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetPixelShader(IDirect3DPixelShader9** ppShader)
{
    if (!ppShader)
    {
        return D3DERR_INVALIDCALL;
    }
    *ppShader = _ps;
    if (_ps) { _ps->AddRef(); }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetPixelShaderConstantF(UINT reg_idx, const float* data, UINT count)
{
    D3DSW_TRACE_STATE("SetPixelShaderConstantF", "reg={}, count={}", reg_idx, count);
    if (!data || reg_idx + count > 224)
    {
        return D3DERR_INVALIDCALL;
    }
    std::memcpy(&_psConstF[reg_idx][0], data, count * 4 * sizeof(float));
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    if (!pConstantData || StartRegister + Vector4fCount > 224)
    {
        return D3DERR_INVALIDCALL;
    }
    std::memcpy(pConstantData, &_psConstF[StartRegister][0], Vector4fCount * 4 * sizeof(float));
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetPixelShaderConstantI(UINT reg_idx, const int* data, UINT count)
{
    D3DSW_TRACE_STATE("SetPixelShaderConstantI", "reg={}, count={}", reg_idx, count);
    if (!data || reg_idx + count > 16)
    {
        return D3DERR_INVALIDCALL;
    }
    std::memcpy(&_psConstI[reg_idx][0], data, count * 4 * sizeof(Int32));
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    if (!pConstantData || StartRegister + Vector4iCount > 16)
    {
        return D3DERR_INVALIDCALL;
    }
    std::memcpy(pConstantData, &_psConstI[StartRegister][0], Vector4iCount * 4 * sizeof(Int32));
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetPixelShaderConstantB(UINT reg_idx, const BOOL* data, UINT count)
{
    D3DSW_TRACE_STATE("SetPixelShaderConstantB", "reg={}, count={}", reg_idx, count);
    if (!data || reg_idx + count > 16)
    {
        return D3DERR_INVALIDCALL;
    }
    for (Uint i = 0; i < count; ++i)
    {
        _psConstB[reg_idx + i] = data[i] ? 1u : 0u;
    }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetPixelShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
    if (!pConstantData || StartRegister + BoolCount > 16)
    {
        return D3DERR_INVALIDCALL;
    }
    for (Uint i = 0; i < BoolCount; ++i)
    {
        pConstantData[i] = _psConstB[StartRegister + i] ? TRUE : FALSE;
    }
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::DrawRectPatch(UINT, const float*, const D3DRECTPATCH_INFO*) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::DrawTriPatch(UINT, const float*, const D3DTRIPATCH_INFO*) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::DeletePatch(UINT) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery)
{
    if (!ppQuery) { return D3DERR_INVALIDCALL; }
    switch (Type)
    {
    case D3DQUERYTYPE_EVENT:
    case D3DQUERYTYPE_OCCLUSION:
    case D3DQUERYTYPE_TIMESTAMP:
    case D3DQUERYTYPE_TIMESTAMPDISJOINT:
    case D3DQUERYTYPE_TIMESTAMPFREQ:
        *ppQuery = new D3D9QuerySW(this, Type);
        return S_OK;
    default:
        *ppQuery = nullptr;
        return D3DERR_NOTAVAILABLE;
    }
}

void D3D9DeviceSW::SnapshotDrawState(D3D9SW_DRAW_STATE& out, D3DPRIMITIVETYPE primitiveType) const
{
    for (Uint i = 0; i < SW_D3D9_MAX_STREAMS; ++i)
    {
        out.streams[i]   = { _streams[i].buffer, _streams[i].offset, _streams[i].stride };
        out.streamFreq[i] = _streamFreq[i];
    }
    out.indices       = _indices;
    out.vertexDecl    = _vertexDecl;
    out.fvf           = _fvf;
    out.primitiveType = primitiveType;

    out.vs       = _vs;
    out.ps       = _ps;
    out.vsConstF = &_vsConstF[0][0];
    out.psConstF = &_psConstF[0][0];

    out.ffVsFn   = nullptr; out.ffVsRefl = nullptr;
    out.ffPsFn   = nullptr; out.ffPsRefl = nullptr;
    if (!_vs)
    {
        auto matMul = [](const D3DMATRIX& a, const D3DMATRIX& b) 
        {
            D3DMATRIX c{};
            for (Int r = 0; r < 4; ++r)
                for (Int col = 0; col < 4; ++col)
                    for (Int k = 0; k < 4; ++k)
                        c.m[r][col] += a.m[r][k] * b.m[k][col];
            return c;
        };

        auto* cb = const_cast<Float(*)[4]>(&_vsConstF[0]);
        auto packMat = [&](const D3DMATRIX& m, Uint32 startRow) 
        {
            for (Int col = 0; col < 4; ++col)
                for (Int row = 0; row < 4; ++row)
                    cb[startRow + col][row] = m.m[row][col];
        };

        D3DMATRIX worldView = matMul(_transforms[D3DTS_WORLD], _transforms[D3DTS_VIEW]);
        D3DMATRIX wvp       = matMul(worldView, _transforms[D3DTS_PROJECTION]);
        packMat(wvp, 0);  

        Bool hasNormal = (_fvf & D3DFVF_NORMAL) != 0;
        if (_vertexDecl)
        {
            for (Uint i = 0; i < _vertexDecl->Count(); ++i)
            {
                if (_vertexDecl->Elements()[i].Usage == D3DDECLUSAGE_NORMAL)
                {
                    hasNormal = true;
                    break;
                }
            }
        }

        Bool lightingOn = (_rs[D3DRS_LIGHTING] != FALSE) && hasNormal;
        if (lightingOn || (_rs[D3DRS_FOGENABLE] && _rs[D3DRS_FOGVERTEXMODE] != D3DFOG_NONE))
        {
            packMat(worldView, 4);
        }

        if (lightingOn)
        {
            for (Int col = 0; col < 3; ++col)
            {
                cb[8 + col][0] = worldView.m[0][col];
                cb[8 + col][1] = worldView.m[1][col];
                cb[8 + col][2] = worldView.m[2][col];
                cb[8 + col][3] = 0.f;
            }

            {
                DWORD amb = _rs[D3DRS_AMBIENT];
                cb[12][0] = ((amb >> 16) & 0xFF) / 255.f;
                cb[12][1] = ((amb >>  8) & 0xFF) / 255.f;
                cb[12][2] = ((amb >>  0) & 0xFF) / 255.f;
                cb[12][3] = 1.f;
            }

            cb[13][0] = _material.Ambient.r;  cb[13][1] = _material.Ambient.g;
            cb[13][2] = _material.Ambient.b;  cb[13][3] = _material.Ambient.a;
            cb[14][0] = _material.Diffuse.r;  cb[14][1] = _material.Diffuse.g;
            cb[14][2] = _material.Diffuse.b;  cb[14][3] = _material.Diffuse.a;
            cb[15][0] = _material.Specular.r; cb[15][1] = _material.Specular.g;
            cb[15][2] = _material.Specular.b; cb[15][3] = _material.Specular.a;
            cb[16][0] = _material.Emissive.r; cb[16][1] = _material.Emissive.g;
            cb[16][2] = _material.Emissive.b; cb[16][3] = _material.Emissive.a;
            cb[17][0] = _material.Power;
            cb[17][1] = cb[17][2] = cb[17][3] = 0.f;

            const D3DMATRIX& view = _transforms[D3DTS_VIEW];
            auto xformPoint = [&](const D3DVECTOR& p) -> D3DVECTOR {
                return { p.x * view.m[0][0] + p.y * view.m[1][0] + p.z * view.m[2][0] + view.m[3][0],
                         p.x * view.m[0][1] + p.y * view.m[1][1] + p.z * view.m[2][1] + view.m[3][1],
                         p.x * view.m[0][2] + p.y * view.m[1][2] + p.z * view.m[2][2] + view.m[3][2] };
            };
            auto xformVec = [&](const D3DVECTOR& v) -> D3DVECTOR {
                return { v.x * view.m[0][0] + v.y * view.m[1][0] + v.z * view.m[2][0],
                         v.x * view.m[0][1] + v.y * view.m[1][1] + v.z * view.m[2][1],
                         v.x * view.m[0][2] + v.y * view.m[1][2] + v.z * view.m[2][2] };
            };
            auto norm3 = [](D3DVECTOR v) -> D3DVECTOR {
                float len = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
                if (len > 1e-6f) { v.x /= len; v.y /= len; v.z /= len; }
                return v;
            };

            for (Uint32 i = 0; i < SW_D3D9_MAX_LIGHTS; ++i)
            {
                if (!_lightEnabled[i]) 
                { 
                    continue; 
                }

                const D3DLIGHT9& l = _lights[i];
                Uint32 base = 18 + i * 8;

                D3DVECTOR posEye = xformPoint(l.Position);
                D3DVECTOR dirEye = xformVec(l.Direction);
                cb[base + 0][0] = posEye.x; cb[base + 0][1] = posEye.y;
                cb[base + 0][2] = posEye.z; cb[base + 0][3] = 0.f;
                if (l.Type == D3DLIGHT_DIRECTIONAL)
                {
                    D3DVECTOR L = norm3({ -dirEye.x, -dirEye.y, -dirEye.z });
                    cb[base + 1][0] = L.x; cb[base + 1][1] = L.y;
                    cb[base + 1][2] = L.z; cb[base + 1][3] = 0.f;
                }
                else
                {
                    D3DVECTOR nd = norm3(dirEye);
                    cb[base + 1][0] = nd.x; cb[base + 1][1] = nd.y;
                    cb[base + 1][2] = nd.z; cb[base + 1][3] = 0.f;
                }

                cb[base + 2][0] = l.Ambient.r;  cb[base + 2][1] = l.Ambient.g;
                cb[base + 2][2] = l.Ambient.b;  cb[base + 2][3] = l.Ambient.a;
                cb[base + 3][0] = l.Diffuse.r;  cb[base + 3][1] = l.Diffuse.g;
                cb[base + 3][2] = l.Diffuse.b;  cb[base + 3][3] = l.Diffuse.a;
                cb[base + 4][0] = l.Specular.r; cb[base + 4][1] = l.Specular.g;
                cb[base + 4][2] = l.Specular.b; cb[base + 4][3] = l.Specular.a;
                cb[base + 5][0] = l.Attenuation0; cb[base + 5][1] = l.Attenuation1;
                cb[base + 5][2] = l.Attenuation2; cb[base + 5][3] = l.Range;
                cb[base + 6][0] = std::cos(l.Theta * 0.5f);
                cb[base + 6][1] = std::cos(l.Phi   * 0.5f);
                cb[base + 6][2] = l.Falloff;
                cb[base + 6][3] = 0.f;
            }
        }

        if (_rs[D3DRS_FOGENABLE] && _rs[D3DRS_FOGVERTEXMODE] != D3DFOG_NONE)
        {
            Float fogStart   = std::bit_cast<Float>(_rs[D3DRS_FOGSTART]);
            Float fogEnd     = std::bit_cast<Float>(_rs[D3DRS_FOGEND]);
            Float fogDensity = std::bit_cast<Float>(_rs[D3DRS_FOGDENSITY]);
            Float invDenom   = (fogEnd != fogStart) ? 1.f / (fogEnd - fogStart) : 0.f;
            DWORD mode = _rs[D3DRS_FOGVERTEXMODE];
            if (mode == D3DFOG_EXP || mode == D3DFOG_EXP2)
            {
                cb[100][2] = (mode == D3DFOG_EXP) ? fogDensity * 1.4427f : fogDensity;
            }
            else
            {
                cb[100][0] = fogStart;
                cb[100][1] = fogEnd;
                cb[100][3] = invDenom;
            }
        }

        if (auto* e = GetFFVS()) { out.ffVsFn = e->fn; out.ffVsRefl = &e->refl; }
    }
    if (!_ps)
    {
        if (auto* e = GetFFPS()) { out.ffPsFn = e->fn; out.ffPsRefl = &e->refl; }
    }

    for (Uint s = 0; s < SW_D3D9_MAX_TEXTURE_STAGES; ++s)
    {
        out.textures[s] = _textures[s];
        std::memcpy(out.samplerStates[s], _samplerStates[s], sizeof(_samplerStates[s]));
    }

    Uint maxRT = 0;
    for (Uint i = 0; i < SW_D3D9_MAX_RTS; ++i)
    {
        out.renderTargets[i] = _renderTargets[i];
        if (_renderTargets[i]) { maxRT = i + 1; }
    }
    out.numRenderTargets = maxRT;
    out.depthStencil  = _currentDSV;

    out.viewport       = _viewport;
    out.scissor        = _scissor;
    out.scissorEnabled = _scissorEnabled;
    out.rs             = _rs;
    out.stats          = const_cast<SWPipelineStatistics*>(&_stats);
}

//Thread-local slot so the FF parse callback can access the pre-built shader
//without needing global state or changing the ShaderJIT API
static thread_local D3DSW_ParsedShader* t_ffPendingShader = nullptr;
static Bool FFShaderParseFn(const void*, Usize, D3DSW_ParsedShader& out)
{
    if (!t_ffPendingShader) 
    { 
        return false; 
    }
    out = *t_ffPendingShader;
    return true;
}

D3D9DeviceSW::FFCacheEntry* D3D9DeviceSW::GetFFVS() const
{
    DWORD fvf = _fvf;
    Bool hasNormal = (fvf & D3DFVF_NORMAL) != 0;
    if (_vertexDecl)
    {
        Bool hasDiff = false, hasSpec = false; 
        Uint numTex = 0;
        hasNormal = false;
        for (Uint i = 0; i < _vertexDecl->Count(); ++i)
        {
            const D3DVERTEXELEMENT9& e = _vertexDecl->Elements()[i];
            if (e.Usage == D3DDECLUSAGE_COLOR   && e.UsageIndex == 0) { hasDiff   = true; }
            if (e.Usage == D3DDECLUSAGE_COLOR   && e.UsageIndex == 1) { hasSpec   = true; }
            if (e.Usage == D3DDECLUSAGE_TEXCOORD)                     { numTex    = std::max(numTex, (Uint)e.UsageIndex + 1); }
            if (e.Usage == D3DDECLUSAGE_NORMAL)                       { hasNormal = true; }
        }
        fvf = D3DFVF_XYZ;
        if (hasDiff)    { fvf |= D3DFVF_DIFFUSE; }
        if (hasSpec)    { fvf |= D3DFVF_SPECULAR; }
        if (hasNormal)  { fvf |= D3DFVF_NORMAL; }
        fvf |= (numTex << D3DFVF_TEXCOUNT_SHIFT);
    }

    Uint numTex = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

    FFVSKey vsKey{};
    vsKey.fvf           = fvf;
    vsKey.numTexCoords  = numTex;
    vsKey.lightingEnabled = (_rs[D3DRS_LIGHTING] != FALSE) && hasNormal ? 1 : 0;
    vsKey.specularEnabled = _rs[D3DRS_SPECULARENABLE]                   ? 1 : 0;
    if (_rs[D3DRS_FOGENABLE])
    {
        vsKey.fogVertexMode = static_cast<Uint8>(_rs[D3DRS_FOGVERTEXMODE] & 0x3u);
    }
    if (vsKey.lightingEnabled)
    {
        for (Uint32 i = 0; i < SW_D3D9_MAX_LIGHTS; ++i)
        {
            if (_lightEnabled[i])
            {
                vsKey.lightMask      |= static_cast<Uint8>(1u << i);
                vsKey.lightTypes[i]   = static_cast<Uint8>(_lights[i].Type);
            }
        }
        if (_rs[D3DRS_COLORVERTEX])
        {
            vsKey.colorVertex = 1;
            vsKey.diffuseSrc  = static_cast<Uint8>(_rs[D3DRS_DIFFUSEMATERIALSOURCE]  & 0x3u);
            vsKey.specularSrc = static_cast<Uint8>(_rs[D3DRS_SPECULARMATERIALSOURCE] & 0x3u);
            vsKey.ambientSrc  = static_cast<Uint8>(_rs[D3DRS_AMBIENTMATERIALSOURCE]  & 0x3u);
            vsKey.emissiveSrc = static_cast<Uint8>(_rs[D3DRS_EMISSIVEMATERIALSOURCE] & 0x3u);
        }
    }

    auto it = _ffVsCache.find(vsKey);
    if (it != _ffVsCache.end()) 
    { 
        return &it->second; 
    }

    FFCacheEntry entry{};
    SynthesizeFFVS(vsKey, entry.refl);

    t_ffPendingShader = &entry.refl;
    entry.fn = GetShaderJIT().GetOrCompile(&vsKey, sizeof(vsKey),
                                           D3DSW_ShaderType::Vertex,
                                           &FFShaderParseFn,
                                           D3DSW_ShaderFrontend::D3D9);
    t_ffPendingShader = nullptr;

    if (!entry.fn) 
    { 
        return nullptr; 
    }
    _ffVsCache[vsKey] = std::move(entry);
    return &_ffVsCache[vsKey];
}

D3D9DeviceSW::FFCacheEntry* D3D9DeviceSW::GetFFPS() const
{
    //Determine what the active VS actually outputs, the FF PS inputs must match.
    //When a custom VS is bound, read directly from its reflection so the FF PS
    //declares the inputs that the custom VS actually provides.
    Bool hasDiffuse    = false;
    Uint numTexCoords  = 0;
    if (_vs)
    {
        for (const auto& e : _vs->GetReflection().outputs)
        {
            if (std::strcmp(e.name, "COLOR") == 0 && e.semanticIndex == 0) { hasDiffuse   = true; }
            if (std::strcmp(e.name, "TEXCOORD") == 0) { numTexCoords = std::max(numTexCoords, e.semanticIndex + 1u); }
        }
    }
    else
    {
        // FF VS: derive from FVF / vertex declaration, same logic as GetFFVS.
        DWORD fvf = _fvf;
        Bool hasNormal = (fvf & D3DFVF_NORMAL) != 0;
        numTexCoords   = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
        hasDiffuse     = (fvf & D3DFVF_DIFFUSE) != 0;
        if (_vertexDecl)
        {
            hasDiffuse = false; numTexCoords = 0; hasNormal = false;
            for (UINT i = 0; i < _vertexDecl->Count(); ++i)
            {
                const D3DVERTEXELEMENT9& e = _vertexDecl->Elements()[i];
                if (e.Usage == D3DDECLUSAGE_COLOR   && e.UsageIndex == 0) { hasDiffuse   = true; }
                if (e.Usage == D3DDECLUSAGE_TEXCOORD)                     { numTexCoords = std::max(numTexCoords, (UINT)e.UsageIndex + 1); }
                if (e.Usage == D3DDECLUSAGE_NORMAL)                       { hasNormal    = true; }
            }
        }

        Bool lightingOn = (_rs[D3DRS_LIGHTING] != FALSE) && hasNormal;
        if (lightingOn) 
        { 
            hasDiffuse = true; 
        }
    }

    FFPSKey psKey{};
    psKey.hasDiffuse   = hasDiffuse   ? 1 : 0;
    psKey.numTexCoords = static_cast<Uint8>(numTexCoords);
    for (Uint s = 0; s < SW_D3D9_MAX_TEXTURE_STAGES; ++s)
    {
        auto& sk       = psKey.stages[s];
        sk.colorOp     = _tss[s][D3DTSS_COLOROP];
        sk.colorArg1   = _tss[s][D3DTSS_COLORARG1];
        sk.colorArg2   = _tss[s][D3DTSS_COLORARG2];
        sk.alphaOp     = _tss[s][D3DTSS_ALPHAOP];
        sk.alphaArg1   = _tss[s][D3DTSS_ALPHAARG1];
        sk.alphaArg2   = _tss[s][D3DTSS_ALPHAARG2];
        sk.hasTexture  = (_textures[s] != nullptr) ? 1 : 0;
    }

    auto it = _ffPsCache.find(psKey);
    if (it != _ffPsCache.end()) 
    { 
        return &it->second; 
    }

    FFCacheEntry entry{};
    SynthesizeFFPS(psKey, entry.refl);

    t_ffPendingShader = &entry.refl;
    entry.fn = GetShaderJIT().GetOrCompile(&psKey, sizeof(psKey),
                                           D3DSW_ShaderType::Pixel,
                                           &FFShaderParseFn,
                                           D3DSW_ShaderFrontend::D3D9);
    t_ffPendingShader = nullptr;

    if (!entry.fn) 
    { 
        return nullptr; 
    }
    _ffPsCache[psKey] = std::move(entry);
    return &_ffPsCache[psKey];
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetConvolutionMonoKernel(UINT, UINT, float*, float*) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::ComposeRects(IDirect3DSurface9*, IDirect3DSurface9*, IDirect3DVertexBuffer9*, UINT, IDirect3DVertexBuffer9*, D3DCOMPOSERECTSOP, INT, INT) { return D3DERR_NOTAVAILABLE; }

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::PresentEx(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion, DWORD /*dwFlags*/)
{
    D3DSW_TRACE_PRESENT("PresentEx");
    return Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetGPUThreadPriority(INT* pPriority) { if (pPriority) { *pPriority = 0; } return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetGPUThreadPriority(INT) { return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::WaitForVBlank(UINT) { return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CheckResourceResidency(IDirect3DResource9**, UINT32) { return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::SetMaximumFrameLatency(UINT) { return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetMaximumFrameLatency(UINT* pMaxLatency) { if (pMaxLatency) { *pMaxLatency = 3; } return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CheckDeviceState(HWND) { return S_OK; }

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateRenderTargetEx(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* /*pSharedHandle*/, DWORD /*Usage*/)
{
    return CreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, nullptr);
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateOffscreenPlainSurfaceEx(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* /*pSharedHandle*/, DWORD /*Usage*/)
{
    return CreateOffscreenPlainSurface(Width, Height, Format, Pool, ppSurface, nullptr);
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::CreateDepthStencilSurfaceEx(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* /*pSharedHandle*/, DWORD /*Usage*/)
{
    return CreateDepthStencilSurface(Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, nullptr);
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::ResetEx(D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* /*pFullscreenDisplayMode*/)
{
    return Reset(pPresentationParameters);
}

HRESULT STDMETHODCALLTYPE D3D9DeviceSW::GetDisplayModeEx(UINT iSwapChain, D3DDISPLAYMODEEX* pMode, D3DDISPLAYROTATION* pRotation)
{
    if (pRotation) 
    { 
        *pRotation = D3DDISPLAYROTATION_IDENTITY; 
    }

    if (!pMode)    
    { 
        return S_OK; 
    }

    D3DDISPLAYMODE base{};
    HRESULT hr = GetDisplayMode(iSwapChain, &base);
    if (FAILED(hr)) 
    { 
        return hr; 
    }

    pMode->Size             = sizeof(D3DDISPLAYMODEEX);
    pMode->Width            = base.Width;
    pMode->Height           = base.Height;
    pMode->RefreshRate      = base.RefreshRate;
    pMode->Format           = base.Format;
    pMode->ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
    return S_OK;
}

}
