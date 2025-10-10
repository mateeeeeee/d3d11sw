#pragma once

#include "unknown_impl.h"

namespace d3d11sw {


template <typename... Interfaces>
class DeviceChildImpl : public UnknownImpl<Interfaces...> 
{
public:
    explicit DeviceChildImpl(ID3D11Device* device) : m_device(device) {}

    void STDMETHODCALLTYPE GetDevice(ID3D11Device** ppDevice) override {
        if (ppDevice) {
            *ppDevice = m_device;
            if (m_device) m_device->AddRef();
        }
    }

    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void* pData) override {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown* pData) override {
        return E_NOTIMPL;
    }

protected:
    ID3D11Device* m_device;
};

}
