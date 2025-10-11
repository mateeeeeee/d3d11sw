#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class D3D11CommandListSW final : public DeviceChildImpl<ID3D11CommandList>
{
public:
    explicit D3D11CommandListSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    UINT STDMETHODCALLTYPE GetContextFlags() override;
};

}
