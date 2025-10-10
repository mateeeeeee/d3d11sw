#include "misc/query.h"

MarsQuery::MarsQuery(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE MarsQuery::GetDataSize()
{
    return 0;
}

void STDMETHODCALLTYPE MarsQuery::GetDesc(D3D11_QUERY_DESC* pDesc)
{
    if (pDesc) *pDesc = {};
}

void STDMETHODCALLTYPE MarsQuery::GetDesc1(D3D11_QUERY_DESC1* pDesc)
{
    if (pDesc) *pDesc = {};
}

MarsPredicate::MarsPredicate(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE MarsPredicate::GetDataSize()
{
    return 0;
}

void STDMETHODCALLTYPE MarsPredicate::GetDesc(D3D11_QUERY_DESC* pDesc)
{
    if (pDesc) *pDesc = {};
}

MarsCounter::MarsCounter(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE MarsCounter::GetDataSize()
{
    return 0;
}

void STDMETHODCALLTYPE MarsCounter::GetDesc(D3D11_COUNTER_DESC* pDesc)
{
    if (pDesc) *pDesc = {};
}
