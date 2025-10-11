#include "misc/query.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11QuerySW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11Query1))
    {
        *ppv = static_cast<ID3D11Query1*>(this);
    }
    else if (riid == __uuidof(ID3D11Query))
    {
        *ppv = static_cast<ID3D11Query*>(this);
    }
    else if (riid == __uuidof(ID3D11Asynchronous))
    {
        *ppv = static_cast<ID3D11Asynchronous*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceChild))
    {
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11PredicateSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11Predicate))
    {
        *ppv = static_cast<ID3D11Predicate*>(this);
    }
    else if (riid == __uuidof(ID3D11Query))
    {
        *ppv = static_cast<ID3D11Query*>(this);
    }
    else if (riid == __uuidof(ID3D11Asynchronous))
    {
        *ppv = static_cast<ID3D11Asynchronous*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceChild))
    {
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11CounterSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11Counter))
    {
        *ppv = static_cast<ID3D11Counter*>(this);
    }
    else if (riid == __uuidof(ID3D11Asynchronous))
    {
        *ppv = static_cast<ID3D11Asynchronous*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceChild))
    {
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

D3D11QuerySW::D3D11QuerySW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE D3D11QuerySW::GetDataSize()
{
    return 0;
}

void STDMETHODCALLTYPE D3D11QuerySW::GetDesc(D3D11_QUERY_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE D3D11QuerySW::GetDesc1(D3D11_QUERY_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

D3D11PredicateSW::D3D11PredicateSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE D3D11PredicateSW::GetDataSize()
{
    return 0;
}

void STDMETHODCALLTYPE D3D11PredicateSW::GetDesc(D3D11_QUERY_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

D3D11CounterSW::D3D11CounterSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE D3D11CounterSW::GetDataSize()
{
    return 0;
}

void STDMETHODCALLTYPE D3D11CounterSW::GetDesc(D3D11_COUNTER_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

}
