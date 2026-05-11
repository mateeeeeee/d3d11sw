#include <algorithm>
#include <numeric>
#include "d3d9/resources/volume_texture.h"
#include "d3d9/device/device.h"
#include "core/util/format.h"
#include "core/common/trace.h"


namespace d3dsw {

static Uint ComputeVolumeMipLevels(Uint w, Uint h, Uint d)
{
    Uint levels = 1;
    while (w > 1 || h > 1 || d > 1)
    {
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
        d = std::max(1u, d / 2);
        ++levels;
    }
    return levels;
}

D3D9VolumeTextureSW::D3D9VolumeTextureSW(D3D9DeviceSW* device,
                                         UINT width, UINT height, UINT depth,
                                         UINT levels, DWORD usage, D3DFORMAT d3dFormat,
                                         DXGI_FORMAT dxgiFormat, D3DPOOL pool)
    : D3D9ResourceImpl(device), _width(width), _height(height), _depth(depth),
      _usage(usage), _d3dFmt(d3dFormat), _dxgiFmt(dxgiFormat), _pool(pool)
{
    Uint actualLevels = (levels == 0) ? ComputeVolumeMipLevels(width, height, depth) : levels;

    // Compute total storage needed and per-mip offsets
    std::vector<Usize> offsets(actualLevels);
    Usize total = 0;
    for (Uint m = 0; m < actualLevels; ++m)
    {
        Uint w = std::max(1u, width  >> m);
        Uint h = std::max(1u, height >> m);
        Uint d = std::max(1u, depth  >> m);
        offsets[m] = total;
        total += static_cast<Usize>(GetRowPitch(dxgiFormat, w)) * h * d;
    }

    _storage.assign(total, 0u);

    _volumes.reserve(actualLevels);
    for (Uint m = 0; m < actualLevels; ++m)
    {
        Uint w = std::max(1u, width  >> m);
        Uint h = std::max(1u, height >> m);
        Uint d = std::max(1u, depth  >> m);
        Uint rp = static_cast<Uint>(GetRowPitch(dxgiFormat, w));
        Uint sp = rp * h;
        Uint ps = static_cast<Uint>(GetFormatStride(dxgiFormat));
        Uint8* ptr = _storage.data() + offsets[m];
        _volumes.push_back(new D3D9VolumeSW(device, ptr, w, h, d, rp, sp, ps, d3dFormat, dxgiFormat));
    }
}

D3D9VolumeTextureSW::~D3D9VolumeTextureSW()
{
    for (D3D9VolumeSW* v : _volumes) { v->Release(); }
}

HRESULT STDMETHODCALLTYPE D3D9VolumeTextureSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) 
    { 
        return E_POINTER; 
    }

    if (riid == IID_IUnknown          ||
        riid == IID_IDirect3DResource9     ||
        riid == IID_IDirect3DBaseTexture9  ||
        riid == IID_IDirect3DVolumeTexture9)
    {
        *ppv = static_cast<IDirect3DVolumeTexture9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE D3D9VolumeTextureSW::GetLevelDesc(UINT Level, D3DVOLUME_DESC* pDesc)
{
    if (Level >= _volumes.size() || !pDesc) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    return _volumes[Level]->GetDesc(pDesc);
}

HRESULT STDMETHODCALLTYPE D3D9VolumeTextureSW::GetVolumeLevel(UINT Level, IDirect3DVolume9** ppVolumeLevel)
{
    if (Level >= _volumes.size() || !ppVolumeLevel) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    _volumes[Level]->AddRef();
    *ppVolumeLevel = _volumes[Level];
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9VolumeTextureSW::LockBox(UINT Level, D3DLOCKED_BOX* pLockedVolume,
                                                       const D3DBOX* pBox, DWORD Flags)
{
    D3DSW_TRACE_MAP("IDirect3DVolumeTexture9::LockBox", "Level={}", Level);
    if (Level >= _volumes.size()) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    return _volumes[Level]->LockBox(pLockedVolume, pBox, Flags);
}

HRESULT STDMETHODCALLTYPE D3D9VolumeTextureSW::UnlockBox(UINT Level)
{
    D3DSW_TRACE_MAP("IDirect3DVolumeTexture9::UnlockBox", "Level={}", Level);
    if (Level >= _volumes.size()) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    return _volumes[Level]->UnlockBox();
}

void D3D9VolumeTextureSW::FillSRV(SW_SRV& out) const
{
    out = {};
    if (_volumes.empty()) 
    { 
        return; 
    }

    const D3D9VolumeSW* base = _volumes[0];
    out.data        = base->DataPtr();
    out.format      = static_cast<Uint>(_dxgiFmt);
    out.stride      = base->PixStride();
    out.mipLevels   = static_cast<Uint>(_volumes.size());
    out.sampleCount = 1;

    for (Uint m = 0; m < out.mipLevels && m < SW_MAX_MIP_LEVELS; ++m)
    {
        const D3D9VolumeSW* vol = _volumes[m];
        out.mips[m].width      = vol->Width();
        out.mips[m].height     = vol->Height();
        out.mips[m].depth      = vol->Depth();
        out.mips[m].rowPitch   = vol->RowPitch();
        out.mips[m].slicePitch = vol->SlicePitch();
        const auto* mipPtr  = static_cast<const Uint8*>(vol->DataPtr());
        const auto* basePtr = static_cast<const Uint8*>(base->DataPtr());
        out.mipOffsets[m] = (m == 0) ? 0u : static_cast<Uint>(mipPtr - basePtr);
    }
}

}
