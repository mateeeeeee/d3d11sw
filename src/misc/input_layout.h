#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class D3D11InputLayoutSW final : public DeviceChildImpl<ID3D11InputLayout>
{
public:
    explicit D3D11InputLayoutSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
};

}
