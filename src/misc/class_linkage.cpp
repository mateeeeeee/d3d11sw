#include "misc/class_linkage.h"

MarsClassLinkage::MarsClassLinkage(ID3D11Device* device)
    : DeviceChildImpl(device) {}

HRESULT STDMETHODCALLTYPE MarsClassLinkage::GetClassInstance(LPCSTR pClassInstanceName, UINT InstanceIndex, ID3D11ClassInstance** ppInstance)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsClassLinkage::CreateClassInstance(LPCSTR pClassTypeName, UINT ConstantBufferOffset, UINT ConstantVectorOffset, UINT TextureOffset, UINT SamplerOffset, ID3D11ClassInstance** ppInstance)
{
    return E_NOTIMPL;
}
