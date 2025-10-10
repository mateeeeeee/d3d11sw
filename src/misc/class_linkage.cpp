#include "misc/class_linkage.h"

namespace d3d11sw {


Direct3D11ClassLinkageSW::Direct3D11ClassLinkageSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

HRESULT STDMETHODCALLTYPE Direct3D11ClassLinkageSW::GetClassInstance(LPCSTR pClassInstanceName, UINT InstanceIndex, ID3D11ClassInstance** ppInstance)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE Direct3D11ClassLinkageSW::CreateClassInstance(LPCSTR pClassTypeName, UINT ConstantBufferOffset, UINT ConstantVectorOffset, UINT TextureOffset, UINT SamplerOffset, ID3D11ClassInstance** ppInstance)
{
    return E_NOTIMPL;
}

}
