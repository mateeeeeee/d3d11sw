#pragma once
#include <vector>
#include <dxgiformat.h>
#include "d3d9/common/d3d9_headers.h"
#include "d3d9/common/resource_impl.h"
#include "d3d9/resources/volume.h"
#include "core/shaders/shader_abi.h"

namespace d3dsw {

class D3D9DeviceSW;

class D3D9VolumeTextureSW final : public D3D9ResourceImpl<IDirect3DVolumeTexture9>
{
public:
    D3D9VolumeTextureSW(D3D9DeviceSW* device, UINT width, UINT height, UINT depth,
                        UINT levels, DWORD usage, D3DFORMAT d3dFormat,
                        DXGI_FORMAT dxgiFormat, D3DPOOL pool);
    ~D3D9VolumeTextureSW() override;

    void FillSRV(SW_SRV& out) const;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return D3DRTYPE_VOLUMETEXTURE; }

    // IDirect3DBaseTexture9
    DWORD STDMETHODCALLTYPE SetLOD(DWORD lod) override { DWORD old = _lod; _lod = lod; return old; }
    DWORD STDMETHODCALLTYPE GetLOD() override { return _lod; }
    DWORD STDMETHODCALLTYPE GetLevelCount() override { return static_cast<DWORD>(_volumes.size()); }
    HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(D3DTEXTUREFILTERTYPE f) override { _autoGen = f; return S_OK; }
    D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE GetAutoGenFilterType() override { return _autoGen; }
    void STDMETHODCALLTYPE GenerateMipSubLevels() override {}

    // IDirect3DVolumeTexture9
    HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DVOLUME_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE GetVolumeLevel(UINT Level, IDirect3DVolume9** ppVolumeLevel) override;
    HRESULT STDMETHODCALLTYPE LockBox(UINT Level, D3DLOCKED_BOX* pLockedVolume, const D3DBOX* pBox, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE UnlockBox(UINT Level) override;
    HRESULT STDMETHODCALLTYPE AddDirtyBox(const D3DBOX*) override { return S_OK; }

private:
    Uint                            _width    = 0;
    Uint                            _height   = 0;
    Uint                            _depth    = 0;
    DWORD                           _usage    = 0;
    D3DFORMAT                       _d3dFmt   = D3DFMT_UNKNOWN;
    DXGI_FORMAT                     _dxgiFmt  = DXGI_FORMAT_UNKNOWN;
    D3DPOOL                         _pool     = D3DPOOL_DEFAULT;
    DWORD                           _lod      = 0;
    D3DTEXTUREFILTERTYPE            _autoGen  = D3DTEXF_POINT;
    std::vector<Uint8>              _storage;   
    std::vector<D3D9VolumeSW*>      _volumes;   
};

}
