#pragma once

#include "common/device_child_impl.h"

class MarsUnorderedAccessView
    : public DeviceChildImpl<ID3D11UnorderedAccessView1,
                             ID3D11UnorderedAccessView,
                             ID3D11View,
                                                          ID3D11DeviceChild>
                             {
public:
    MarsUnorderedAccessView(ID3D11Device* device);

    void STDMETHODCALLTYPE GetResource(ID3D11Resource** ppResource) override;

    void STDMETHODCALLTYPE GetDesc(D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc) override;

    void STDMETHODCALLTYPE GetDesc1(D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc) override;
};
