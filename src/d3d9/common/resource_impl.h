#pragma once
#include "d3d9/common/d3d9_headers.h"
#include "core/common/common.h"
#include "core/common/unknown_impl.h"
#include "core/common/private_data.h"

namespace d3dsw {

// CRTP-style base for D3D9 resource COM wrappers. T must extend IDirect3DResource9.
template <typename T> requires std::is_base_of_v<IDirect3DResource9, T>
class D3D9ResourceImpl : public T, private UnknownBase
{
public:
    explicit D3D9ResourceImpl(IDirect3DDevice9* device) : _device(device)
    {
        if (_device)
        {
            _device->AddRef();
        }
    }

    virtual ~D3D9ResourceImpl()
    {
        if (_device)
        {
            _device->Release();
        }
    }

    ULONG STDMETHODCALLTYPE AddRef() override  { return AddRefImpl(); }
    ULONG STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }

    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) override
    {
        if (!ppDevice)
        {
            return D3DERR_INVALIDCALL;
        }
        *ppDevice = _device;
        if (_device)
        {
            _device->AddRef();
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, const void* pData, DWORD SizeOfData, DWORD Flags) override
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

    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, void* pData, DWORD* pSizeOfData) override
    {
        UINT size = pSizeOfData ? static_cast<UINT>(*pSizeOfData) : 0u;
        HRESULT hr = _privateData.GetData(guid, &size, pData);
        if (pSizeOfData) { *pSizeOfData = static_cast<DWORD>(size); }
        if (hr == DXGI_ERROR_NOT_FOUND)  { return D3DERR_NOTFOUND;  }
        if (hr == DXGI_ERROR_MORE_DATA)  { return D3DERR_MOREDATA;  }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID guid) override
    {
        HRESULT hr = _privateData.FreeData(guid);
        return (hr == DXGI_ERROR_NOT_FOUND) ? D3DERR_NOTFOUND : hr;
    }

    DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override
    {
        DWORD old = _priority;
        _priority = PriorityNew;
        return old;
    }
    DWORD STDMETHODCALLTYPE GetPriority() override { return _priority; }
    void  STDMETHODCALLTYPE PreLoad()     override {}

protected:
    IDirect3DDevice9* _device   = nullptr;
    DWORD             _priority = 0;

private:
    PrivateDataStore  _privateData;
};

}
