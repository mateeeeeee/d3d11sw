#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11ClassLinkageSW : public DeviceChildImpl<ID3D11ClassLinkage, ID3D11DeviceChild>
{
public:
    explicit Direct3D11ClassLinkageSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE GetClassInstance(LPCSTR pClassInstanceName, UINT InstanceIndex, ID3D11ClassInstance** ppInstance) override;
    HRESULT STDMETHODCALLTYPE CreateClassInstance(LPCSTR pClassTypeName, UINT ConstantBufferOffset, UINT ConstantVectorOffset, UINT TextureOffset, UINT SamplerOffset, ID3D11ClassInstance** ppInstance) override;
};

}
