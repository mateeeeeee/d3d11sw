#pragma once
#include "device/device_child_impl.h"

namespace d3d11sw {


class D3D11RasterizerStateSW final : public DeviceChildImpl<ID3D11RasterizerState2>
{
public:
    explicit D3D11RasterizerStateSW(ID3D11Device* device);

    HRESULT Init(const D3D11_RASTERIZER_DESC2* pDesc);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    void STDMETHODCALLTYPE GetDesc(D3D11_RASTERIZER_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_RASTERIZER_DESC1* pDesc) override;
    void STDMETHODCALLTYPE GetDesc2(D3D11_RASTERIZER_DESC2* pDesc) override;

private:
    D3D11_RASTERIZER_DESC2 _desc{};
};

}
