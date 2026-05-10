#pragma once
#include "d3d11/common/d3d11_headers.h"

#include "d3d11/device/device_child_impl.h"

namespace d3dsw {


class D3D11ClassLinkageSW final : public DeviceChildImpl<ID3D11ClassLinkage>
{
public:
    explicit D3D11ClassLinkageSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE GetClassInstance(LPCSTR pClassInstanceName, UINT InstanceIndex, ID3D11ClassInstance** ppInstance) override;
    HRESULT STDMETHODCALLTYPE CreateClassInstance(LPCSTR pClassTypeName, UINT ConstantBufferOffset, UINT ConstantVectorOffset, UINT TextureOffset, UINT SamplerOffset, ID3D11ClassInstance** ppInstance) override;
};

}
