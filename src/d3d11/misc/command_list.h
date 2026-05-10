#pragma once
#include "d3d11/common/d3d11_headers.h"

#include "d3d11/device/device_child_impl.h"

namespace d3dsw {


class D3D11CommandListSW final : public DeviceChildImpl<ID3D11CommandList>
{
public:
    explicit D3D11CommandListSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    UINT STDMETHODCALLTYPE GetContextFlags() override;
};

}
