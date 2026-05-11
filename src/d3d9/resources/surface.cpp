#include <cstring>
#include "d3d9/resources/surface.h"
#include "d3d9/device/device.h"
#include "core/util/format.h"
#include "core/common/trace.h"


namespace d3dsw {

D3D9SurfaceSW::D3D9SurfaceSW(D3D9DeviceSW* device, UINT width, UINT height,
                             D3DFORMAT d3dFormat, DXGI_FORMAT dxgiFormat, D3DRESOURCETYPE resourceType)
    : _device(device), _width(width), _height(height),
      _d3dFormat(d3dFormat), _dxgiFormat(dxgiFormat), _resourceType(resourceType)
{
    _pixStride = GetFormatStride(dxgiFormat);
    _rowPitch  = GetRowPitch(dxgiFormat, width);
    Usize bytes = static_cast<Usize>(_rowPitch) * height;
    _data = new Uint8[bytes]();
    _ownsData = true;
}

D3D9SurfaceSW::D3D9SurfaceSW(D3D9DeviceSW* device, UINT width, UINT height,
                             D3DFORMAT d3dFormat, DXGI_FORMAT dxgiFormat, D3DRESOURCETYPE resourceType,
                             Uint8* externalData)
    : _device(device), _width(width), _height(height),
      _d3dFormat(d3dFormat), _dxgiFormat(dxgiFormat), _resourceType(resourceType)
{
    _pixStride = GetFormatStride(dxgiFormat);
    _rowPitch  = GetRowPitch(dxgiFormat, width);
    _data     = externalData;
    _ownsData = false;
}

D3D9SurfaceSW::~D3D9SurfaceSW()
{
    if (_ownsData) 
    { 
        delete[] _data; 
    }
    delete[] _shadowBuf;
}

HRESULT STDMETHODCALLTYPE D3D9SurfaceSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }
    if (riid == IID_IUnknown ||
        riid == IID_IDirect3DResource9 ||
        riid == IID_IDirect3DSurface9)
    {
        *ppv = static_cast<IDirect3DSurface9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE D3D9SurfaceSW::GetDevice(IDirect3DDevice9** ppDevice)
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

HRESULT STDMETHODCALLTYPE D3D9SurfaceSW::SetPrivateData(REFGUID guid, const void* pData, DWORD SizeOfData, DWORD Flags)
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
HRESULT STDMETHODCALLTYPE D3D9SurfaceSW::GetPrivateData(REFGUID guid, void* pData, DWORD* pSizeOfData)
{
    Uint size = pSizeOfData ? static_cast<Uint>(*pSizeOfData) : 0u;
    HRESULT hr = _privateData.GetData(guid, &size, pData);
    if (pSizeOfData) 
    { 
        *pSizeOfData = static_cast<DWORD>(size); 
    }
    if (hr == DXGI_ERROR_NOT_FOUND) { return D3DERR_NOTFOUND; }
    if (hr == DXGI_ERROR_MORE_DATA) { return D3DERR_MOREDATA; }
    return hr;
}
HRESULT STDMETHODCALLTYPE D3D9SurfaceSW::FreePrivateData(REFGUID guid)
{
    HRESULT hr = _privateData.FreeData(guid);
    return (hr == DXGI_ERROR_NOT_FOUND) ? D3DERR_NOTFOUND : hr;
}
DWORD   STDMETHODCALLTYPE D3D9SurfaceSW::SetPriority(DWORD PriorityNew) { DWORD old = _priority; _priority = PriorityNew; return old; }
DWORD   STDMETHODCALLTYPE D3D9SurfaceSW::GetPriority() { return _priority; }
void    STDMETHODCALLTYPE D3D9SurfaceSW::PreLoad() {}
D3DRESOURCETYPE STDMETHODCALLTYPE D3D9SurfaceSW::GetType() { return _resourceType; }

HRESULT STDMETHODCALLTYPE D3D9SurfaceSW::GetContainer(REFIID, void**) { return D3DERR_NOTAVAILABLE; }

HRESULT STDMETHODCALLTYPE D3D9SurfaceSW::GetDesc(D3DSURFACE_DESC* pDesc)
{
    if (!pDesc)
    {
        return D3DERR_INVALIDCALL;
    }
    pDesc->Format          = _d3dFormat;
    pDesc->Type            = _resourceType;
    pDesc->Usage           = 0;
    pDesc->Pool            = D3DPOOL_DEFAULT;
    pDesc->MultiSampleType = D3DMULTISAMPLE_NONE;
    pDesc->MultiSampleQuality = 0;
    pDesc->Width           = _width;
    pDesc->Height          = _height;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9SurfaceSW::LockRect(D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags)
{
    D3DSW_TRACE_MAP("IDirect3DSurface9::LockRect", "{}x{}", _width, _height);
    if (!pLockedRect)
    {
        return D3DERR_INVALIDCALL;
    }
    if (_locked)
    {
        return D3DERR_INVALIDCALL;
    }
    _locked    = true;
    _lockFlags = Flags;
    if (_d3dFormat == D3DFMT_R8G8B8)
    {
        if (!_shadowBuf)
        {
            _shadowBuf = new Uint8[_width * 3 * _height]();
        }
        Uint byteOffset = pRect ? (pRect->top * _width * 3 + pRect->left * 3) : 0;
        pLockedRect->Pitch = static_cast<INT>(_width * 3);
        pLockedRect->pBits = _shadowBuf + byteOffset;
        return S_OK;
    }
    if (_d3dFormat == D3DFMT_R3G3B2)
    {
        if (!_shadowBuf)
        {
            _shadowBuf = new Uint8[_width * _height]();
        }
        Uint byteOffset = pRect ? (pRect->top * _width + pRect->left) : 0;
        pLockedRect->Pitch = static_cast<Int>(_width);
        pLockedRect->pBits = _shadowBuf + byteOffset;
        return S_OK;
    }

    Uint8* base = _data;
    if (pRect)
    {
        base += static_cast<Usize>(pRect->top) * _rowPitch + static_cast<Usize>(pRect->left) * _pixStride;
    }
    pLockedRect->Pitch = static_cast<INT>(_rowPitch);
    pLockedRect->pBits = base;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9SurfaceSW::UnlockRect()
{
    D3DSW_TRACE_MAP("IDirect3DSurface9::UnlockRect");
    if (!_locked)
    {
        return D3DERR_INVALIDCALL;
    }
    _locked    = false;
    _lockFlags = 0;
    if (_d3dFormat == D3DFMT_R8G8B8 && _shadowBuf)
    {
        for (Uint y = 0; y < _height; ++y)
        {
            const Uint8* src = _shadowBuf + y * _width * 3;
            Uint8* dst = _data + y * _rowPitch;
            for (Uint x = 0; x < _width; ++x)
            {
                dst[x * 4 + 0] = src[x * 3 + 0];  // B
                dst[x * 4 + 1] = src[x * 3 + 1];  // G
                dst[x * 4 + 2] = src[x * 3 + 2];  // R
                dst[x * 4 + 3] = 0xFF;
            }
        }
    }
    if (_d3dFormat == D3DFMT_R3G3B2 && _shadowBuf)
    {
        for (Uint y = 0; y < _height; ++y)
        {
            const Uint8* src = _shadowBuf + y * _width;
            Uint8* dst = _data + y * _rowPitch;
            for (Uint x = 0; x < _width; ++x)
            {
                Uint8 p = src[x];
                Uint8 r3 = (p >> 5) & 0x7;
                Uint8 g3 = (p >> 2) & 0x7;
                Uint8 b2 = (p >> 0) & 0x3;
                // Expand to 8-bit: replicate high bits into low bits
                dst[x * 4 + 0] = static_cast<Uint8>((b2 << 6) | (b2 << 4) | (b2 << 2) | b2);  // B
                dst[x * 4 + 1] = static_cast<Uint8>((g3 << 5) | (g3 << 2) | (g3 >> 1));         // G
                dst[x * 4 + 2] = static_cast<Uint8>((r3 << 5) | (r3 << 2) | (r3 >> 1));         // R
                dst[x * 4 + 3] = 0xFF;
            }
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9SurfaceSW::GetDC(HDC*) { return D3DERR_NOTAVAILABLE; }
HRESULT STDMETHODCALLTYPE D3D9SurfaceSW::ReleaseDC(HDC) { return D3DERR_NOTAVAILABLE; }

}
