#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class D3D11ClassLinkageSW final : public DeviceChildImpl<ID3D11ClassLinkage>
{
public:
    explicit D3D11ClassLinkageSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE GetClassInstance(LPCSTR pClassInstanceName, UINT InstanceIndex, ID3D11ClassInstance** ppInstance) override;
    HRESULT STDMETHODCALLTYPE CreateClassInstance(LPCSTR pClassTypeName, UINT ConstantBufferOffset, UINT ConstantVectorOffset, UINT TextureOffset, UINT SamplerOffset, ID3D11ClassInstance** ppInstance) override;
};

}
