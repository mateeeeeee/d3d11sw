#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11BlendStateSW : public DeviceChildImpl<ID3D11BlendState1>
{
public:
    explicit Direct3D11BlendStateSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) final;

    void STDMETHODCALLTYPE GetDesc(D3D11_BLEND_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_BLEND_DESC1* pDesc) override;
};

}
