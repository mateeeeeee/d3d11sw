#pragma once
#include <vector>
#include "d3d9/common/d3d9_headers.h"
#include "d3d9/common/resource_impl.h"
#include "d3d9/resources/surface.h"
#include "core/shaders/shader_abi.h"

namespace d3dsw {

class D3D9DeviceSW;

class D3D9CubeTextureSW final : public D3D9ResourceImpl<IDirect3DCubeTexture9>
{
public:
    D3D9CubeTextureSW(D3D9DeviceSW* device, UINT edgeLength, UINT levels,
                      DWORD usage, D3DFORMAT d3dFormat, DXGI_FORMAT dxgiFormat, D3DPOOL pool);
    ~D3D9CubeTextureSW() override;

    void FillSRV(SW_SRV& out) const;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return D3DRTYPE_CUBETEXTURE; }

    // IDirect3DBaseTexture9
    DWORD   STDMETHODCALLTYPE SetLOD(DWORD lod) override { DWORD old = _lod; _lod = lod; return old; }
    DWORD   STDMETHODCALLTYPE GetLOD() override { return _lod; }
    DWORD   STDMETHODCALLTYPE GetLevelCount() override { return static_cast<DWORD>(_levels); }
    HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(D3DTEXTUREFILTERTYPE f) override { _autoGen = f; return S_OK; }
    D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE GetAutoGenFilterType() override { return _autoGen; }
    void    STDMETHODCALLTYPE GenerateMipSubLevels() override {}

    // IDirect3DCubeTexture9
    HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE GetCubeMapSurface(D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface9** ppSurf) override;
    HRESULT STDMETHODCALLTYPE LockRect(D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level) override;
    HRESULT STDMETHODCALLTYPE AddDirtyRect(D3DCUBEMAP_FACES, const RECT*) override { return S_OK; }

private:
    Uint32                          _edgeLength  = 0;
    Uint32                          _levels      = 0;
    Uint32                          _usage       = 0;
    D3DFORMAT                       _d3dFmt      = D3DFMT_UNKNOWN;
    DXGI_FORMAT                     _dxgiFmt     = DXGI_FORMAT_UNKNOWN;
    D3DPOOL                         _pool        = D3DPOOL_DEFAULT;
    Uint32                          _lod         = 0;
    D3DTEXTUREFILTERTYPE            _autoGen     = D3DTEXF_POINT;
    std::vector<Uint8>              _storage;
    Uint32                          _faceStride  = 0;
    std::vector<D3D9SurfaceSW*>     _surfaces;

private:
    D3D9SurfaceSW* GetSurface(Uint face, Uint level) const;
};

}
