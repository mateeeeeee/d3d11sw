#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11InputLayoutSW : public DeviceChildImpl<ID3D11InputLayout>
{
public:
    explicit Direct3D11InputLayoutSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) final;
};

}
