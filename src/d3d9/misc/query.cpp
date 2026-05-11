#include <chrono>
#include <cstring>
#include "d3d9/misc/query.h"
#include "d3d9/device/device.h"
#include "core/common/trace.h"

namespace d3dsw {

D3D9QuerySW::D3D9QuerySW(D3D9DeviceSW* device, D3DQUERYTYPE type)
    : _device(device), _type(type)
{
    if (_device) 
    { 
        _device->AddRef(); 
    }
}

HRESULT STDMETHODCALLTYPE D3D9QuerySW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) 
    { 
        return E_POINTER; 
    }

    if (riid == IID_IUnknown || riid == IID_IDirect3DQuery9)
    {
        *ppv = static_cast<IDirect3DQuery9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE D3D9QuerySW::GetDevice(IDirect3DDevice9** ppDevice)
{
    if (!ppDevice) 
    { 
        return D3DERR_INVALIDCALL; 
    }

    *ppDevice = static_cast<IDirect3DDevice9*>(_device);
    if (_device) 
    { 
        _device->AddRef(); 
    }
    return S_OK;
}

D3DQUERYTYPE STDMETHODCALLTYPE D3D9QuerySW::GetType() { return _type; }

DWORD STDMETHODCALLTYPE D3D9QuerySW::GetDataSize()
{
    switch (_type)
    {
    case D3DQUERYTYPE_EVENT:              return sizeof(BOOL);
    case D3DQUERYTYPE_OCCLUSION:          return sizeof(DWORD);
    case D3DQUERYTYPE_TIMESTAMP:          return sizeof(UINT64);
    case D3DQUERYTYPE_TIMESTAMPDISJOINT:  return sizeof(BOOL);
    case D3DQUERYTYPE_TIMESTAMPFREQ:      return sizeof(UINT64);
    default:                              return 0;
    }
}

HRESULT STDMETHODCALLTYPE D3D9QuerySW::Issue(DWORD dwIssueFlags)
{
    D3DSW_TRACE_STATE("IDirect3DQuery9::Issue", "Flags={:#x}", dwIssueFlags);
    if (dwIssueFlags & D3DISSUE_END)
    {
        _issued = true;
        if (_type == D3DQUERYTYPE_TIMESTAMP)
        {
            auto now = std::chrono::steady_clock::now();
            _timestamp = static_cast<Uint64>(now.time_since_epoch().count());
            using Period = std::chrono::steady_clock::period;
            _timestamp = _timestamp * Period::num * (1'000'000'000 / Period::den);
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9QuerySW::GetData(void* pData, DWORD dwSize, DWORD /*dwGetDataFlags*/)
{
    D3DSW_TRACE_STATE("IDirect3DQuery9::GetData", "Type={}", static_cast<Uint32>(_type));
    if (!_issued)
    {
        return S_FALSE;
    }

    if (!pData || dwSize < GetDataSize())
    {
        return D3DERR_INVALIDCALL;
    }

    switch (_type)
    {
    case D3DQUERYTYPE_EVENT:
    {
        BOOL done = TRUE;
        std::memcpy(pData, &done, sizeof(BOOL));
        return S_OK;
    }
    case D3DQUERYTYPE_OCCLUSION:
    {
        DWORD samples = 0;
        std::memcpy(pData, &samples, sizeof(DWORD));
        return S_OK;
    }
    case D3DQUERYTYPE_TIMESTAMP:
    {
        std::memcpy(pData, &_timestamp, sizeof(UINT64));
        return S_OK;
    }
    case D3DQUERYTYPE_TIMESTAMPDISJOINT:
    {
        BOOL disjoint = FALSE;
        std::memcpy(pData, &disjoint, sizeof(BOOL));
        return S_OK;
    }
    case D3DQUERYTYPE_TIMESTAMPFREQ:
    {
        UINT64 freq = 1'000'000'000;
        std::memcpy(pData, &freq, sizeof(UINT64));
        return S_OK;
    }
    default:
        return S_FALSE;
    }
}

}
