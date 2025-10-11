#pragma once

#include "common/unknown_impl.h"

namespace d3d11sw {


class Direct3D11MultithreadSW : public ID3D11Multithread, private UnknownBase
{
public:
    ULONG STDMETHODCALLTYPE AddRef() override  { return AddRefImpl(); }
    ULONG STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv)
            return E_POINTER;

        *ppv = nullptr;

        if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11Multithread))
        {
            *ppv = static_cast<ID3D11Multithread*>(this);
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    void STDMETHODCALLTYPE Enter() override;
    void STDMETHODCALLTYPE Leave() override;
    BOOL STDMETHODCALLTYPE SetMultithreadProtected(BOOL bMTProtect) override;
    BOOL STDMETHODCALLTYPE GetMultithreadProtected() override;
};


}
