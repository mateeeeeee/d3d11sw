#include "misc/query.h"

namespace d3d11sw {


Direct3D11QuerySW::Direct3D11QuerySW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE Direct3D11QuerySW::GetDataSize()
{
    return 0;
}

void STDMETHODCALLTYPE Direct3D11QuerySW::GetDesc(D3D11_QUERY_DESC* pDesc)
{
    if (pDesc) *pDesc = {};
}

void STDMETHODCALLTYPE Direct3D11QuerySW::GetDesc1(D3D11_QUERY_DESC1* pDesc)
{
    if (pDesc) *pDesc = {};
}

Direct3D11PredicateSW::Direct3D11PredicateSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE Direct3D11PredicateSW::GetDataSize()
{
    return 0;
}

void STDMETHODCALLTYPE Direct3D11PredicateSW::GetDesc(D3D11_QUERY_DESC* pDesc)
{
    if (pDesc) *pDesc = {};
}

Direct3D11CounterSW::Direct3D11CounterSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE Direct3D11CounterSW::GetDataSize()
{
    return 0;
}

void STDMETHODCALLTYPE Direct3D11CounterSW::GetDesc(D3D11_COUNTER_DESC* pDesc)
{
    if (pDesc) *pDesc = {};
}

}
