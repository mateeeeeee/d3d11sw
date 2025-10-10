#pragma once
#include "common/device_child_impl.h"

namespace d3d11sw {



class Direct3D11QuerySW : public DeviceChildImpl<ID3D11Query1, ID3D11Query, ID3D11Asynchronous, ID3D11DeviceChild>
{
public:
    explicit Direct3D11QuerySW(ID3D11Device* device);

    UINT STDMETHODCALLTYPE GetDataSize() override;
    void STDMETHODCALLTYPE GetDesc(D3D11_QUERY_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_QUERY_DESC1* pDesc) override;
};

class Direct3D11PredicateSW : public DeviceChildImpl<ID3D11Predicate, ID3D11Query, ID3D11Asynchronous, ID3D11DeviceChild>
{
public:
    explicit Direct3D11PredicateSW(ID3D11Device* device);

    UINT STDMETHODCALLTYPE GetDataSize() override;
    void STDMETHODCALLTYPE GetDesc(D3D11_QUERY_DESC* pDesc) override;
};

class Direct3D11CounterSW : public DeviceChildImpl<ID3D11Counter, ID3D11Asynchronous, ID3D11DeviceChild>
{
public:
    explicit Direct3D11CounterSW(ID3D11Device* device);

    UINT STDMETHODCALLTYPE GetDataSize() override;
    void STDMETHODCALLTYPE GetDesc(D3D11_COUNTER_DESC* pDesc) override;
};

}
