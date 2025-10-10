#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11BufferSW : public DeviceChildImpl<ID3D11Buffer, ID3D11Resource, ID3D11DeviceChild>
{
public:
    explicit Direct3D11BufferSW(ID3D11Device* device);

    void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension) override;
    void STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) override;
    UINT STDMETHODCALLTYPE GetEvictionPriority() override;

    void STDMETHODCALLTYPE GetDesc(D3D11_BUFFER_DESC* pDesc) override;
};

}
