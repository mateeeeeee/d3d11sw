#pragma once
#include "d3d11/common/d3d11_headers.h"

#include "core/common/unknown_impl.h"
#include "core/common/private_data.h"

namespace d3dsw {


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
            {
                _device->AddRef();
            }
        }
    }

    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData) override
    {
        return _privateData.GetData(guid, pDataSize, pData);
    }

    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void* pData) override
    {
        return _privateData.SetData(guid, DataSize, pData);
    }

    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown* pData) override
    {
        return _privateData.SetInterface(guid, pData);
    }

protected:
    ID3D11Device* _device;
    PrivateDataStore _privateData;
};


}
