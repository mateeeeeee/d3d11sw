#pragma once
#include <dxgiformat.h>
#include "d3d9/common/d3d9_headers.h"
#include "core/common/common.h"
#include "core/common/unknown_impl.h"
#include "core/common/private_data.h"

namespace d3dsw {

class D3D9DeviceSW;

class D3D9SurfaceSW final : public IDirect3DSurface9, private UnknownBase
{
public:
    D3D9SurfaceSW(D3D9DeviceSW* device, UINT width, UINT height,
                  D3DFORMAT d3dFormat, DXGI_FORMAT dxgiFormat, D3DRESOURCETYPE resourceType);
    //Non-owning
    D3D9SurfaceSW(D3D9DeviceSW* device, UINT width, UINT height,
                  D3DFORMAT d3dFormat, DXGI_FORMAT dxgiFormat, D3DRESOURCETYPE resourceType,
                  Uint8* externalData);
    ~D3D9SurfaceSW() override;

    Uint8*      DataPtr()           { return _data; }
    const Uint8* DataPtr() const    { return _data; }
    Uint        Width() const       { return _width; }
    Uint        Height() const      { return _height; }
    Uint        RowPitch() const    { return _rowPitch; }
    Uint        PixStride() const   { return _pixStride; }
    D3DFORMAT   D3DFormat() const   { return _d3dFormat; }
    DXGI_FORMAT DxgiFormat() const  { return _dxgiFormat; }

    // IUnknown
    ULONG   STDMETHODCALLTYPE AddRef() override  { return AddRefImpl(); }
    ULONG   STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    // IDirect3DResource9
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) override;
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID, const void*, DWORD, DWORD) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID, void*, DWORD*) override;
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID) override;
    DWORD   STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override;
    DWORD   STDMETHODCALLTYPE GetPriority() override;
    void    STDMETHODCALLTYPE PreLoad() override;
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override;

    // IDirect3DSurface9
    HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) override;
    HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE UnlockRect() override;
    HRESULT STDMETHODCALLTYPE GetDC(HDC* phdc) override;
    HRESULT STDMETHODCALLTYPE ReleaseDC(HDC hdc) override;

private:
    D3D9DeviceSW* _device       = nullptr;
    Uint8*        _data         = nullptr;
    Uint          _width        = 0;
    Uint          _height       = 0;
    Uint          _rowPitch     = 0;
    Uint          _pixStride    = 0;
    D3DFORMAT     _d3dFormat    = D3DFMT_UNKNOWN;
    DXGI_FORMAT   _dxgiFormat   = DXGI_FORMAT_UNKNOWN;
    D3DRESOURCETYPE _resourceType = D3DRTYPE_SURFACE;
    Uint32         _priority     = 0;
    Uint32         _lockFlags    = 0;
    Bool          _locked       = false;
    Uint8*        _shadowBuf    = nullptr;  
    Bool          _ownsData     = true;      
    PrivateDataStore _privateData;
};

}
