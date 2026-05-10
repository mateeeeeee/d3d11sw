#include "d3d9/resources/texture.h"
#include "d3d9/resources/surface.h"
#include "d3d9/device/device.h"
#include "core/util/format.h"
#include <algorithm>
#include <cstdint>

namespace d3dsw {

static UINT ComputeMipLevels(UINT width, UINT height)
{
    UINT levels = 1;
    while (width > 1 || height > 1)
    {
        width  = std::max(1u, width  / 2);
        height = std::max(1u, height / 2);
        ++levels;
    }
    return levels;
}

D3D9TextureSW::D3D9TextureSW(D3D9DeviceSW* device, UINT width, UINT height, UINT levels,
                             DWORD usage, D3DFORMAT d3dFormat, DXGI_FORMAT dxgiFormat, D3DPOOL pool)
    : D3D9ResourceImpl(device), _usage(usage), _d3dFmt(d3dFormat), _pool(pool)
{
    UINT actualLevels = (levels == 0) ? ComputeMipLevels(width, height) : levels;
    _surfaces.reserve(actualLevels);
    for (UINT i = 0; i < actualLevels; ++i)
    {
        UINT w = std::max(1u, width  >> i);
        UINT h = std::max(1u, height >> i);
        auto* surf = new D3D9SurfaceSW(device, w, h, d3dFormat, dxgiFormat, D3DRTYPE_SURFACE);
        _surfaces.push_back(surf);   // owns one ref
    }
}

D3D9TextureSW::~D3D9TextureSW()
{
    for (auto* surf : _surfaces)
    {
        surf->Release();
    }
}

HRESULT STDMETHODCALLTYPE D3D9TextureSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }
    if (riid == IID_IUnknown ||
        riid == IID_IDirect3DResource9 ||
        riid == IID_IDirect3DBaseTexture9 ||
        riid == IID_IDirect3DTexture9)
    {
        *ppv = static_cast<IDirect3DTexture9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

D3DRESOURCETYPE STDMETHODCALLTYPE D3D9TextureSW::GetType() { return D3DRTYPE_TEXTURE; }

DWORD STDMETHODCALLTYPE D3D9TextureSW::SetLOD(DWORD LODNew)
{
    if (_pool != D3DPOOL_MANAGED)
    {
        return 0;   // D3D9: LOD only settable on MANAGED textures; returns 0 otherwise
    }
    DWORD old = _lod;
    _lod = std::min<DWORD>(LODNew, static_cast<DWORD>(_surfaces.size() - 1));
    return old;
}
DWORD STDMETHODCALLTYPE D3D9TextureSW::GetLOD()        { return _lod; }
DWORD STDMETHODCALLTYPE D3D9TextureSW::GetLevelCount() { return static_cast<DWORD>(_surfaces.size()); }

HRESULT STDMETHODCALLTYPE D3D9TextureSW::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType)
{
    _autoGen = FilterType;
    return S_OK;
}
D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE D3D9TextureSW::GetAutoGenFilterType() { return _autoGen; }
void STDMETHODCALLTYPE D3D9TextureSW::GenerateMipSubLevels() {}

HRESULT STDMETHODCALLTYPE D3D9TextureSW::GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc)
{
    if (Level >= _surfaces.size() || !pDesc)
    {
        return D3DERR_INVALIDCALL;
    }
    return _surfaces[Level]->GetDesc(pDesc);
}

HRESULT STDMETHODCALLTYPE D3D9TextureSW::GetSurfaceLevel(UINT Level, IDirect3DSurface9** ppSurfaceLevel)
{
    if (Level >= _surfaces.size() || !ppSurfaceLevel)
    {
        if (ppSurfaceLevel) { *ppSurfaceLevel = nullptr; }
        return D3DERR_INVALIDCALL;
    }
    _surfaces[Level]->AddRef();
    *ppSurfaceLevel = _surfaces[Level];
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9TextureSW::LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags)
{
    if (Level >= _surfaces.size())
    {
        return D3DERR_INVALIDCALL;
    }
    return _surfaces[Level]->LockRect(pLockedRect, pRect, Flags);
}

HRESULT STDMETHODCALLTYPE D3D9TextureSW::UnlockRect(UINT Level)
{
    if (Level >= _surfaces.size())
    {
        return D3DERR_INVALIDCALL;
    }
    return _surfaces[Level]->UnlockRect();
}

HRESULT STDMETHODCALLTYPE D3D9TextureSW::AddDirtyRect(const RECT*) { return S_OK; }

void D3D9TextureSW::FillSRV(SW_SRV& out) const
{
    out = {};
    if (_surfaces.empty()) { return; }

    const D3D9SurfaceSW* base = _surfaces[0];
    out.data       = base->DataPtr();
    out.format     = static_cast<unsigned>(base->DxgiFormat());
    out.stride     = base->PixStride();
    out.mipLevels  = static_cast<unsigned>(_surfaces.size());
    out.sampleCount = 1;

    for (unsigned m = 0; m < out.mipLevels && m < SW_MAX_MIP_LEVELS; ++m)
    {
        const D3D9SurfaceSW* surf = _surfaces[m];
        out.mips[m].width      = surf->Width();
        out.mips[m].height     = surf->Height();
        out.mips[m].depth      = 1;
        out.mips[m].rowPitch   = surf->RowPitch();
        out.mips[m].slicePitch = surf->RowPitch() * surf->Height();
        const auto* mipPtr  = static_cast<const uint8_t*>(static_cast<const void*>(surf->DataPtr()));
        const auto* basePtr = static_cast<const uint8_t*>(static_cast<const void*>(base->DataPtr()));
        out.mipOffsets[m] = (m == 0) ? 0u : static_cast<unsigned>(mipPtr - basePtr);
    }

    // D3D9 format swizzle: DXGI stores fewer components; the sample must
    // broadcast/remap to match D3D9 semantics.
    // Constants: swizzle index 4 = 0.0, 5 = 1.0
    switch (_d3dFmt)
    {
    case D3DFMT_L8:   // R8_UNORM → (L,L,L,1)
    case D3DFMT_L16:  // R16_UNORM → (L,L,L,1)
        out.swizzle[0] = 0; out.swizzle[1] = 0; out.swizzle[2] = 0; out.swizzle[3] = 5;
        out.hasSwizzle = 1;
        break;
    case D3DFMT_A8L8: // R8G8_UNORM → R=lum, G=alpha → (L,L,L,A)
        out.swizzle[0] = 0; out.swizzle[1] = 0; out.swizzle[2] = 0; out.swizzle[3] = 1;
        out.hasSwizzle = 1;
        break;
    case D3DFMT_A8:   // A8_UNORM stored in R → (0,0,0,A)
        out.swizzle[0] = 4; out.swizzle[1] = 4; out.swizzle[2] = 4; out.swizzle[3] = 0;
        out.hasSwizzle = 1;
        break;
    case D3DFMT_X8B8G8R8: // R8G8B8A8_UNORM, alpha is undefined → force 1
        out.swizzle[0] = 0; out.swizzle[1] = 1; out.swizzle[2] = 2; out.swizzle[3] = 5;
        out.hasSwizzle = 1;
        break;
    default:
        break;
    }
}

}
