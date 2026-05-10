#pragma once
#include <vector>
#include <dxgiformat.h>
#include "d3d9/common/d3d9_headers.h"
#include "d3d9/common/resource_impl.h"
#include "core/shaders/shader_abi.h"


namespace d3dsw {

class D3D9DeviceSW;
class D3D9SurfaceSW;

class D3D9TextureSW final : public D3D9ResourceImpl<IDirect3DTexture9>
{
public:
    D3D9TextureSW(D3D9DeviceSW* device, UINT width, UINT height, UINT levels,
                  DWORD usage, D3DFORMAT d3dFormat, DXGI_FORMAT dxgiFormat, D3DPOOL pool);
    ~D3D9TextureSW() override;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override;

    // IDirect3DBaseTexture9
    DWORD   STDMETHODCALLTYPE SetLOD(DWORD LODNew) override;
    DWORD   STDMETHODCALLTYPE GetLOD() override;
    DWORD   STDMETHODCALLTYPE GetLevelCount() override;
    HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType) override;
    D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE GetAutoGenFilterType() override;
    void    STDMETHODCALLTYPE GenerateMipSubLevels() override;

    // IDirect3DTexture9
    HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE GetSurfaceLevel(UINT Level, IDirect3DSurface9** ppSurfaceLevel) override;
    HRESULT STDMETHODCALLTYPE LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE UnlockRect(UINT Level) override;
    HRESULT STDMETHODCALLTYPE AddDirtyRect(const RECT* pDirtyRect) override;

    void FillSRV(SW_SRV& out) const;

private:
    std::vector<D3D9SurfaceSW*> _surfaces;       
    Uint32                       _usage    = 0;
    D3DFORMAT                   _d3dFmt   = D3DFMT_UNKNOWN;
    D3DPOOL                     _pool     = D3DPOOL_DEFAULT;
    Uint32                       _lod      = 0;
    D3DTEXTUREFILTERTYPE        _autoGen  = D3DTEXF_LINEAR;
};

}
