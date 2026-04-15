#pragma once

#include "common/device_child_impl.h"
#include "context/pipeline_state.h"

namespace d3d11sw {


class D3D11SW_API D3D11QuerySW final : public DeviceChildImpl<ID3D11Query1>
{
public:
    explicit D3D11QuerySW(ID3D11Device* device);

    HRESULT Init(const D3D11_QUERY_DESC1* pDesc);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    UINT STDMETHODCALLTYPE GetDataSize() override;
    void STDMETHODCALLTYPE GetDesc(D3D11_QUERY_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_QUERY_DESC1* pDesc) override;

    void    Begin(const D3D11SW_PIPELINE_STATISTICS* stats = nullptr);
    void    End(const D3D11SW_PIPELINE_STATISTICS* stats = nullptr);
    HRESULT GetData(void* pData, UINT DataSize);

private:
    D3D11_QUERY_DESC1 _desc{};
    Uint64            _timestamp = 0;
    Bool              _ended = false;
    D3D11SW_PIPELINE_STATISTICS _statsBegin{};
    D3D11SW_PIPELINE_STATISTICS _statsEnd{};
};

class D3D11SW_API D3D11PredicateSW final : public DeviceChildImpl<ID3D11Predicate>
{
public:
    explicit D3D11PredicateSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    UINT STDMETHODCALLTYPE GetDataSize() override;
    void STDMETHODCALLTYPE GetDesc(D3D11_QUERY_DESC* pDesc) override;
};

class D3D11SW_API D3D11CounterSW final : public DeviceChildImpl<ID3D11Counter>
{
public:
    explicit D3D11CounterSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    UINT STDMETHODCALLTYPE GetDataSize() override;
    void STDMETHODCALLTYPE GetDesc(D3D11_COUNTER_DESC* pDesc) override;
};

}
