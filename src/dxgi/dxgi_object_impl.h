#pragma once

#include "common/unknown_impl.h"
#include "common/private_data.h"

namespace d3d11sw {

template <typename T>
class DXGIObjectImpl : public T, private UnknownBase
{
public:
    virtual ~DXGIObjectImpl() = default;

    ULONG STDMETHODCALLTYPE AddRef() override  { return AddRefImpl(); }
    ULONG STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }

    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData) override
    {
        return _privateData.SetData(Name, DataSize, pData);
    }

    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown) override
    {
        return _privateData.SetInterface(Name, pUnknown);
    }

    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData) override
    {
        return _privateData.GetData(Name, pDataSize, pData);
    }

private:
    PrivateDataStore _privateData;
};

}
