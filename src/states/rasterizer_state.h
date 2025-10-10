#pragma once

#include "common/device_child_impl.h"

class MarsRasterizerState : public DeviceChildImpl<ID3D11RasterizerState2, ID3D11RasterizerState1, ID3D11RasterizerState, ID3D11DeviceChild>
{
public:
    MarsRasterizerState(ID3D11Device* device);

    void STDMETHODCALLTYPE GetDesc(D3D11_RASTERIZER_DESC* pDesc) override;

    void STDMETHODCALLTYPE GetDesc1(D3D11_RASTERIZER_DESC1* pDesc) override;

    void STDMETHODCALLTYPE GetDesc2(D3D11_RASTERIZER_DESC2* pDesc) override;
};
