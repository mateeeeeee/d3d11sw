#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class D3D11UnorderedAccessViewSW final : public DeviceChildImpl<ID3D11UnorderedAccessView1>
{
public:
    explicit D3D11UnorderedAccessViewSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    void STDMETHODCALLTYPE GetResource(ID3D11Resource** ppResource) override;
    void STDMETHODCALLTYPE GetDesc(D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc) override;
};

}
