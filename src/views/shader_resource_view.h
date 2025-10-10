#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11ShaderResourceViewSW
    : public DeviceChildImpl<ID3D11ShaderResourceView1,
                             ID3D11ShaderResourceView,
                             ID3D11View,
                             ID3D11DeviceChild>
{
public:
    explicit Direct3D11ShaderResourceViewSW(ID3D11Device* device);

    void STDMETHODCALLTYPE GetResource(ID3D11Resource** ppResource) override;
    void STDMETHODCALLTYPE GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc) override;
};

}
