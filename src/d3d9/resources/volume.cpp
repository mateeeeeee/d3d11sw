#include "d3d9/resources/volume.h"
#include "d3d9/device/device.h"
#include "core/common/trace.h"

namespace d3dsw {

D3D9VolumeSW::D3D9VolumeSW(D3D9DeviceSW* device, Uint8* data,
                           UINT width, UINT height, UINT depth,
                           UINT rowPitch, UINT slicePitch, UINT pixStride,
                           D3DFORMAT d3dFormat, DXGI_FORMAT dxgiFormat)
    : _device(device), _data(data), _width(width), _height(height), _depth(depth),
      _rowPitch(rowPitch), _slicePitch(slicePitch), _pixStride(pixStride),
      _d3dFormat(d3dFormat), _dxgiFormat(dxgiFormat)
{}

HRESULT STDMETHODCALLTYPE D3D9VolumeSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) 
    { 
        return E_POINTER; 
    }

    if (riid == IID_IUnknown || riid == IID_IDirect3DVolume9)
    {
        *ppv = static_cast<IDirect3DVolume9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE D3D9VolumeSW::GetDevice(IDirect3DDevice9** ppDevice)
{
    if (!ppDevice) 
    { 
        return D3DERR_INVALIDCALL; 
    }

    *ppDevice = static_cast<IDirect3DDevice9*>(_device);
    if (_device) 
    { 
        _device->AddRef(); 
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9VolumeSW::SetPrivateData(REFGUID guid, const void* pData, DWORD SizeOfData, DWORD Flags)
{
    if (!pData) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    if (Flags & D3DSPD_IUNKNOWN)
    {
        return _privateData.SetInterface(guid, *static_cast<IUnknown* const*>(pData));
    }
    return _privateData.SetData(guid, SizeOfData, pData);
}

HRESULT STDMETHODCALLTYPE D3D9VolumeSW::GetPrivateData(REFGUID guid, void* pData, DWORD* pSizeOfData)
{
    Uint size = pSizeOfData ? static_cast<Uint>(*pSizeOfData) : 0u;
    HRESULT hr = _privateData.GetData(guid, &size, pData);
    if (pSizeOfData) 
    { 
        *pSizeOfData = static_cast<DWORD>(size); 
    }
    if (hr == DXGI_ERROR_NOT_FOUND) 
    { 
        return D3DERR_NOTFOUND; 
    }
    if (hr == DXGI_ERROR_MORE_DATA) 
    { 
        return D3DERR_MOREDATA; 
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE D3D9VolumeSW::FreePrivateData(REFGUID guid)
{
    HRESULT hr = _privateData.FreeData(guid);
    return (hr == DXGI_ERROR_NOT_FOUND) ? D3DERR_NOTFOUND : hr;
}

HRESULT STDMETHODCALLTYPE D3D9VolumeSW::GetContainer(REFIID riid, void** ppContainer)
{
    if (!ppContainer) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    if (!_device) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    return _device->QueryInterface(riid, ppContainer);
}

HRESULT STDMETHODCALLTYPE D3D9VolumeSW::GetDesc(D3DVOLUME_DESC* pDesc)
{
    if (!pDesc) { return D3DERR_INVALIDCALL; }
    pDesc->Format = _d3dFormat;
    pDesc->Type   = D3DRTYPE_VOLUME;
    pDesc->Usage  = 0;
    pDesc->Pool   = D3DPOOL_DEFAULT;
    pDesc->Width  = _width;
    pDesc->Height = _height;
    pDesc->Depth  = _depth;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9VolumeSW::LockBox(D3DLOCKED_BOX* pLockedVolume, const D3DBOX* pBox, DWORD /*Flags*/)
{
    D3DSW_TRACE_MAP("IDirect3DVolume9::LockBox", "{}x{}x{}", _width, _height, _depth);
    if (!pLockedVolume) 
    { 
        return D3DERR_INVALIDCALL; 
    }

    if (_locked)        
    { 
        return D3DERR_INVALIDCALL; 
    }

    _locked = true;
    Uint8* base = _data;
    if (pBox)
    {
        base += static_cast<Usize>(pBox->Front)  * _slicePitch
              + static_cast<Usize>(pBox->Top)    * _rowPitch
              + static_cast<Usize>(pBox->Left)   * _pixStride;
    }
    pLockedVolume->RowPitch   = static_cast<INT>(_rowPitch);
    pLockedVolume->SlicePitch = static_cast<INT>(_slicePitch);
    pLockedVolume->pBits      = base;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9VolumeSW::UnlockBox()
{
    D3DSW_TRACE_MAP("IDirect3DVolume9::UnlockBox");
    if (!_locked) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    _locked = false;
    return S_OK;
}

}
