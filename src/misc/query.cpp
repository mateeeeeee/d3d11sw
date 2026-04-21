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
    case D3D11_QUERY_PIPELINE_STATISTICS:
    case D3D11_QUERY_OCCLUSION:
    case D3D11_QUERY_OCCLUSION_PREDICATE:
    case D3D11_QUERY_SO_STATISTICS:
    case D3D11_QUERY_SO_OVERFLOW_PREDICATE:
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
    case D3D11_QUERY_PIPELINE_STATISTICS: return sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS);
    case D3D11_QUERY_OCCLUSION:          return sizeof(UINT64);
    case D3D11_QUERY_OCCLUSION_PREDICATE: return sizeof(BOOL);
    case D3D11_QUERY_SO_STATISTICS:      return sizeof(D3D11_QUERY_DATA_SO_STATISTICS);
    case D3D11_QUERY_SO_OVERFLOW_PREDICATE: return sizeof(BOOL);
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

void D3D11QuerySW::Begin(const D3D11SW_PIPELINE_STATISTICS* stats)
{
    _ended = false;
    if (stats)
    {
        _statsBegin = *stats;
    }
}

void D3D11QuerySW::End(const D3D11SW_PIPELINE_STATISTICS* stats)
{
    if (_desc.Query == D3D11_QUERY_TIMESTAMP)
    {
        auto now = std::chrono::steady_clock::now();
        _timestamp = static_cast<Uint64>(now.time_since_epoch().count());
        using period = std::chrono::steady_clock::period;
        _timestamp = _timestamp * period::num * (1'000'000'000 / period::den);
    }
    else if (stats)
    {
        _statsEnd = *stats;
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
    case D3D11_QUERY_PIPELINE_STATISTICS:
    {
        if (DataSize < sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS))
        {
            return E_INVALIDARG;
        }
        D3D11_QUERY_DATA_PIPELINE_STATISTICS data{};
        data.IAVertices    = _statsEnd.iaVertices    - _statsBegin.iaVertices;
        data.IAPrimitives  = _statsEnd.iaPrimitives  - _statsBegin.iaPrimitives;
        data.VSInvocations = _statsEnd.vsInvocations - _statsBegin.vsInvocations;
        data.GSInvocations = _statsEnd.gsInvocations - _statsBegin.gsInvocations;
        data.GSPrimitives  = _statsEnd.gsPrimitives  - _statsBegin.gsPrimitives;
        data.CInvocations  = _statsEnd.cInvocations  - _statsBegin.cInvocations;
        data.CPrimitives   = _statsEnd.cPrimitives   - _statsBegin.cPrimitives;
        data.PSInvocations = _statsEnd.psInvocations - _statsBegin.psInvocations;
        data.HSInvocations = _statsEnd.hsInvocations - _statsBegin.hsInvocations;
        data.DSInvocations = _statsEnd.dsInvocations - _statsBegin.dsInvocations;
        data.CSInvocations = _statsEnd.csInvocations - _statsBegin.csInvocations;
        std::memcpy(pData, &data, sizeof(data));
        return S_OK;
    }
    case D3D11_QUERY_OCCLUSION:
    {
        if (DataSize < sizeof(UINT64))
        {
            return E_INVALIDARG;
        }
        UINT64 count = _statsEnd.occlusionCount - _statsBegin.occlusionCount;
        std::memcpy(pData, &count, sizeof(UINT64));
        return S_OK;
    }
    case D3D11_QUERY_OCCLUSION_PREDICATE:
    {
        if (DataSize < sizeof(BOOL))
        {
            return E_INVALIDARG;
        }
        BOOL visible = (_statsEnd.occlusionCount - _statsBegin.occlusionCount) > 0 ? TRUE : FALSE;
        std::memcpy(pData, &visible, sizeof(BOOL));
        return S_OK;
    }
    case D3D11_QUERY_SO_STATISTICS:
    {
        if (DataSize < sizeof(D3D11_QUERY_DATA_SO_STATISTICS))
        {
            return E_INVALIDARG;
        }
        D3D11_QUERY_DATA_SO_STATISTICS data{};
        data.NumPrimitivesWritten    = _statsEnd.soNumPrimitivesWritten    - _statsBegin.soNumPrimitivesWritten;
        data.PrimitivesStorageNeeded = _statsEnd.soPrimitivesStorageNeeded - _statsBegin.soPrimitivesStorageNeeded;
        std::memcpy(pData, &data, sizeof(data));
        return S_OK;
    }
    case D3D11_QUERY_SO_OVERFLOW_PREDICATE:
    {
        if (DataSize < sizeof(BOOL))
        {
            return E_INVALIDARG;
        }
        UINT64 written = _statsEnd.soNumPrimitivesWritten    - _statsBegin.soNumPrimitivesWritten;
        UINT64 needed  = _statsEnd.soPrimitivesStorageNeeded - _statsBegin.soPrimitivesStorageNeeded;
        BOOL overflowed = (needed > written) ? TRUE : FALSE;
        std::memcpy(pData, &overflowed, sizeof(BOOL));
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
