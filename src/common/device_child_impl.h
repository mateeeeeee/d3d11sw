#pragma once

#include "unknown_impl.h"

namespace d3d11sw {


template <typename T>
class DeviceChildImpl : public T, private UnknownBase
{
public:
    explicit DeviceChildImpl(ID3D11Device* device) : _device(device) {}

    virtual ~DeviceChildImpl() = default;

    ULONG STDMETHODCALLTYPE AddRef() override  { return AddRefImpl(); }
    ULONG STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }

    void STDMETHODCALLTYPE GetDevice(ID3D11Device** ppDevice) override
    {
        if (ppDevice)
        {
            *ppDevice = _device;
            if (_device)
                _device->AddRef();
        }
    }

    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void* pData) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown* pData) override
    {
        return E_NOTIMPL;
    }

protected:
    ID3D11Device* _device;
};


}
