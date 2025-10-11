#include "misc/class_linkage.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11ClassLinkageSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11ClassLinkage))
    {
        *ppv = static_cast<ID3D11ClassLinkage*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceChild))
    {
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

D3D11ClassLinkageSW::D3D11ClassLinkageSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

HRESULT STDMETHODCALLTYPE D3D11ClassLinkageSW::GetClassInstance(LPCSTR pClassInstanceName, UINT InstanceIndex, ID3D11ClassInstance** ppInstance)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11ClassLinkageSW::CreateClassInstance(LPCSTR pClassTypeName, UINT ConstantBufferOffset, UINT ConstantVectorOffset, UINT TextureOffset, UINT SamplerOffset, ID3D11ClassInstance** ppInstance)
{
    return E_NOTIMPL;
}

}
