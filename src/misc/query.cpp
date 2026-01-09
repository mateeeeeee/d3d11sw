#include "misc/query.h"
#include <chrono>
#include <cstring>

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

HRESULT D3D11QuerySW::Init(const D3D11_QUERY_DESC1* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }

    switch (pDesc->Query)
    {
    case D3D11_QUERY_TIMESTAMP:
    case D3D11_QUERY_TIMESTAMP_DISJOINT:
    case D3D11_QUERY_EVENT:
        break;
    default:
        return E_NOTIMPL;
    }

    _desc = *pDesc;
    return S_OK;
}

UINT STDMETHODCALLTYPE D3D11QuerySW::GetDataSize()
{
    switch (_desc.Query)
    {
    case D3D11_QUERY_TIMESTAMP:          return sizeof(UINT64);
    case D3D11_QUERY_TIMESTAMP_DISJOINT: return sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT);
    case D3D11_QUERY_EVENT:              return sizeof(BOOL);
    default:                             return 0;
    }
}

void STDMETHODCALLTYPE D3D11QuerySW::GetDesc(D3D11_QUERY_DESC* pDesc)
{
    if (pDesc)
    {
        pDesc->Query    = _desc.Query;
        pDesc->MiscFlags = _desc.MiscFlags;
    }
}

void STDMETHODCALLTYPE D3D11QuerySW::GetDesc1(D3D11_QUERY_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = _desc;
    }
}

void D3D11QuerySW::Begin()
{
    _ended = false;
}

void D3D11QuerySW::End()
{
    if (_desc.Query == D3D11_QUERY_TIMESTAMP)
    {
        auto now = std::chrono::steady_clock::now();
        _timestamp = static_cast<Uint64>(now.time_since_epoch().count());
        using period = std::chrono::steady_clock::period;
        _timestamp = _timestamp * period::num * (1'000'000'000 / period::den);
    }
    _ended = true;
}

HRESULT D3D11QuerySW::GetData(void* pData, UINT DataSize)
{
    if (!_ended)
    {
        return S_FALSE;
    }

    if (!pData)
    {
        return S_OK;
    }

    switch (_desc.Query)
    {
    case D3D11_QUERY_TIMESTAMP:
    {
        if (DataSize < sizeof(UINT64))
        {
            return E_INVALIDARG;
        }
        std::memcpy(pData, &_timestamp, sizeof(UINT64));
        return S_OK;
    }
    case D3D11_QUERY_TIMESTAMP_DISJOINT:
    {
        if (DataSize < sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT))
        {
            return E_INVALIDARG;
        }
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data{};
        data.Frequency = 1'000'000'000;
        data.Disjoint  = FALSE;
        std::memcpy(pData, &data, sizeof(data));
        return S_OK;
    }
    case D3D11_QUERY_EVENT:
    {
        if (DataSize < sizeof(BOOL))
        {
            return E_INVALIDARG;
        }
        BOOL done = TRUE;
        std::memcpy(pData, &done, sizeof(BOOL));
        return S_OK;
    }
    default:
        return E_NOTIMPL;
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
