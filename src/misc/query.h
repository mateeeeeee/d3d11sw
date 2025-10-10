#pragma once
#include "common/device_child_impl.h"


class MarsQuery : public DeviceChildImpl<ID3D11Query1, ID3D11Query, ID3D11Asynchronous, ID3D11DeviceChild>
{
public:
    MarsQuery(ID3D11Device* device);

    UINT STDMETHODCALLTYPE GetDataSize() override;
    void STDMETHODCALLTYPE GetDesc(D3D11_QUERY_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_QUERY_DESC1* pDesc) override;
};

class MarsPredicate : public DeviceChildImpl<ID3D11Predicate, ID3D11Query, ID3D11Asynchronous, ID3D11DeviceChild>
{
public:
    MarsPredicate(ID3D11Device* device);

    UINT STDMETHODCALLTYPE GetDataSize() override;
    void STDMETHODCALLTYPE GetDesc(D3D11_QUERY_DESC* pDesc) override;
};

class MarsCounter : public DeviceChildImpl<ID3D11Counter, ID3D11Asynchronous, ID3D11DeviceChild>
{
public:
    MarsCounter(ID3D11Device* device);

    UINT STDMETHODCALLTYPE GetDataSize() override;
    void STDMETHODCALLTYPE GetDesc(D3D11_COUNTER_DESC* pDesc) override;
};
