#pragma once

#include "common/device_child_impl.h"

class MarsBuffer : public DeviceChildImpl<ID3D11Buffer, ID3D11Resource, ID3D11DeviceChild>
{
public:
    MarsBuffer(ID3D11Device* device);

    void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension) override;
    void STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) override;
    UINT STDMETHODCALLTYPE GetEvictionPriority() override;

    void STDMETHODCALLTYPE GetDesc(D3D11_BUFFER_DESC* pDesc) override;
};
