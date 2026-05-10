#include <algorithm>
#include <cstring>
#include "d3d9/resources/cube_texture.h"
#include "d3d9/device/device.h"
#include "core/util/format.h"

namespace d3dsw {

static Uint ComputeMipLevels(Uint size)
{
    Uint n = 1;
    while (size > 1) 
    { 
        size >>= 1; ++n; 
    }
    return n;
}

D3D9CubeTextureSW::D3D9CubeTextureSW(D3D9DeviceSW* device, UINT edgeLength, UINT levels,
                                     DWORD usage, D3DFORMAT d3dFormat, DXGI_FORMAT dxgiFormat,
                                     D3DPOOL pool)
    : D3D9ResourceImpl(device), _edgeLength(edgeLength), _usage(usage),
      _d3dFmt(d3dFormat), _dxgiFmt(dxgiFormat), _pool(pool)
{
    Uint actualLevels = (levels == 0) ? ComputeMipLevels(edgeLength) : levels;
    _levels = actualLevels;

    const Uint pixStride = GetFormatStride(dxgiFormat);
    const Uint rowPitch  = GetRowPitch(dxgiFormat, edgeLength);
    _faceStride = 0;
    {
        Uint w = edgeLength;
        for (Uint m = 0; m < actualLevels; ++m)
        {
            Uint rp = GetRowPitch(dxgiFormat, w);
            _faceStride += rp * w;
            w = std::max(1u, w / 2);
        }
    }
    _storage.assign(static_cast<Usize>(6) * _faceStride, 0);

    _surfaces.resize(6 * actualLevels);
    Uint w = edgeLength;
    for (Uint m = 0; m < actualLevels; ++m)
    {
        Uint levelOffset = 0;
        {
            Uint ww = edgeLength;
            for (Uint mm = 0; mm < m; ++mm)
            {
                levelOffset += GetRowPitch(dxgiFormat, ww) * ww;
                ww = std::max(1u, ww / 2);
            }
        }
        for (Uint f = 0; f < 6; ++f)
        {
            Uint8* faceBase = _storage.data() + f * _faceStride + levelOffset;
            _surfaces[f * actualLevels + m] = new D3D9SurfaceSW(
                device, w, w, d3dFormat, dxgiFormat, D3DRTYPE_SURFACE, faceBase);
        }
        w = std::max(1u, w / 2);
    }
}

D3D9CubeTextureSW::~D3D9CubeTextureSW()
{
    for (auto* s : _surfaces) 
    { 
        s->Release(); 
    }
}

D3D9SurfaceSW* D3D9CubeTextureSW::GetSurface(UINT face, UINT level) const
{
    if (face >= 6 || level >= _levels) 
    { 
        return nullptr; 
    }
    return _surfaces[face * _levels + level];
}

HRESULT STDMETHODCALLTYPE D3D9CubeTextureSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) 
    { 
        return E_POINTER; 
    }

    if (riid == IID_IUnknown ||
        riid == IID_IDirect3DResource9 ||
        riid == IID_IDirect3DBaseTexture9 ||
        riid == IID_IDirect3DCubeTexture9)
    {
        *ppv = static_cast<IDirect3DCubeTexture9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE D3D9CubeTextureSW::GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc)
{
    if (Level >= _levels || !pDesc) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    return GetSurface(0, Level)->GetDesc(pDesc);
}

HRESULT STDMETHODCALLTYPE D3D9CubeTextureSW::GetCubeMapSurface(D3DCUBEMAP_FACES FaceType, UINT Level,
                                                                 IDirect3DSurface9** ppSurf)
{
    if (!ppSurf) 
    { 
        return D3DERR_INVALIDCALL; 
    }

    D3D9SurfaceSW* s = GetSurface(static_cast<UINT>(FaceType), Level);
    if (!s) 
    { 
        *ppSurf = nullptr; 
        return D3DERR_INVALIDCALL; 
    }

    s->AddRef();
    *ppSurf = s;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9CubeTextureSW::LockRect(D3DCUBEMAP_FACES FaceType, UINT Level,
                                                       D3DLOCKED_RECT* pLockedRect, const RECT* pRect,
                                                       DWORD Flags)
{
    D3D9SurfaceSW* s = GetSurface(static_cast<UINT>(FaceType), Level);
    if (!s) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    return s->LockRect(pLockedRect, pRect, Flags);
}

HRESULT STDMETHODCALLTYPE D3D9CubeTextureSW::UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level)
{
    D3D9SurfaceSW* s = GetSurface(static_cast<UINT>(FaceType), Level);
    if (!s) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    return s->UnlockRect();
}

void D3D9CubeTextureSW::FillSRV(SW_SRV& out) const
{
    out = {};
    if (_surfaces.empty()) 
    { 
        return; 
    }
    Uint pixStride = GetFormatStride(_dxgiFmt);
    Uint rowPitch  = GetRowPitch(_dxgiFmt, _edgeLength);
    
    out.data        = _storage.data();
    out.format      = static_cast<Uint>(_dxgiFmt);
    out.stride      = pixStride;
    out.mipLevels   = 1;     //todo: sw_sample_cube only uses mip 0
    out.sampleCount = 1;

    out.mips[0].width      = _edgeLength;
    out.mips[0].height     = _edgeLength;
    out.mips[0].depth      = 1;
    out.mips[0].rowPitch   = rowPitch;
    out.mips[0].slicePitch = _faceStride;  
    out.mipOffsets[0] = 0;
}

}
