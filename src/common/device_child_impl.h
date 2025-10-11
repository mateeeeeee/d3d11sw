#pragma once

#include "unknown_impl.h"

namespace d3d11sw {

class Direct3D11DeviceSW;

template <typename... Interfaces>
class DeviceChildImpl : public UnknownImpl<Interfaces...> 
{
public:
    explicit DeviceChildImpl(ID3D11Device* device) : _device(device) {}

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
