#pragma once
#include "common.h"

namespace d3d11sw {


template <typename Type, typename... Rest>
struct FirstOf
{
    using type = Type;
};

template <typename... Ts>
using FirstOfT = typename FirstOf<Ts...>::type;


template <typename... Interfaces>
class UnknownImpl : public FirstOfT<Interfaces...>
{
public:
    virtual ~UnknownImpl() = default;

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return m_refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG count = m_refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (count == 0)
        {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv)
        {
            return E_POINTER;
        }
        *ppv = nullptr;

        if (riid == __uuidof(IUnknown))
        {
            *ppv = static_cast<IUnknown*>(
                static_cast<FirstOfT<Interfaces...>*>(this));
            AddRef();
            return S_OK;
        }

        if ((TryQI<Interfaces>(riid, ppv) || ...)) 
        {
            return S_OK;
        }

        return E_NOINTERFACE;
    }

private:
    std::atomic<ULONG> m_refCount{1};

private:
    template <typename T>
    bool TryQI(REFIID riid, void** ppv) 
    {
        if (riid == __uuidof(T)) 
        {
            *ppv = static_cast<T*>(this);
            AddRef();
            return true;
        }
        return false;
    }
};

}
