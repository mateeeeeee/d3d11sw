#pragma once
#include "d3d11/common/d3d11_headers.h"
#include "d3d11/device/device_child_impl.h"

namespace d3dsw {


class D3D11SamplerStateSW final : public DeviceChildImpl<ID3D11SamplerState>
{
public:
    explicit D3D11SamplerStateSW(ID3D11Device* device);

    HRESULT Init(const D3D11_SAMPLER_DESC* pDesc);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    void STDMETHODCALLTYPE GetDesc(D3D11_SAMPLER_DESC* pDesc) override;

private:
    D3D11_SAMPLER_DESC _desc{};
};

}
