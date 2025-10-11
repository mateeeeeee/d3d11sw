#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11RasterizerStateSW : public DeviceChildImpl<ID3D11RasterizerState2>
{
public:
    explicit Direct3D11RasterizerStateSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) final;

    void STDMETHODCALLTYPE GetDesc(D3D11_RASTERIZER_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_RASTERIZER_DESC1* pDesc) override;
    void STDMETHODCALLTYPE GetDesc2(D3D11_RASTERIZER_DESC2* pDesc) override;
};

}
