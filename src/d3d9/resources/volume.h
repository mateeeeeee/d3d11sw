#pragma once
#include <dxgiformat.h>
#include "d3d9/common/d3d9_headers.h"
#include "core/common/common.h"
#include "core/common/unknown_impl.h"
#include "core/common/private_data.h"

namespace d3dsw {

class D3D9DeviceSW;

class D3D9VolumeSW final : public IDirect3DVolume9, private UnknownBase
{
public:
    D3D9VolumeSW(D3D9DeviceSW* device, Uint8* data,
                 UINT width, UINT height, UINT depth,
                 UINT rowPitch, UINT slicePitch, UINT pixStride,
                 D3DFORMAT d3dFormat, DXGI_FORMAT dxgiFormat);
    ~D3D9VolumeSW() override = default;

    Uint8*      DataPtr()     const { return _data; }
    Uint        Width()       const { return _width; }
    Uint        Height()      const { return _height; }
    Uint        Depth()       const { return _depth; }
    Uint        RowPitch()    const { return _rowPitch; }
    Uint        SlicePitch()  const { return _slicePitch; }
    Uint        PixStride()   const { return _pixStride; }
    DXGI_FORMAT DxgiFormat()  const { return _dxgiFormat; }

    // IUnknown
    ULONG   STDMETHODCALLTYPE AddRef()  override { return AddRefImpl(); }
    ULONG   STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    // IDirect3DVolume9 resource methods
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) override;
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, const void* pData, DWORD SizeOfData, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, void* pData, DWORD* pSizeOfData) override;
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID guid) override;

    // IDirect3DVolume9 methods
    HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) override;
    HRESULT STDMETHODCALLTYPE GetDesc(D3DVOLUME_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE LockBox(D3DLOCKED_BOX* pLockedVolume, const D3DBOX* pBox, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE UnlockBox() override;

private:
    D3D9DeviceSW* _device      = nullptr;
    Uint8*        _data        = nullptr;
    Uint          _width       = 0;
    Uint          _height      = 0;
    Uint          _depth       = 0;
    Uint          _rowPitch    = 0;
    Uint          _slicePitch  = 0;
    Uint          _pixStride   = 0;
    D3DFORMAT     _d3dFormat   = D3DFMT_UNKNOWN;
    DXGI_FORMAT   _dxgiFormat  = DXGI_FORMAT_UNKNOWN;
    Bool          _locked      = false;
    PrivateDataStore _privateData;
};

}
