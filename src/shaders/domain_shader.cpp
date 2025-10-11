#include "shaders/domain_shader.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE Direct3D11DomainShaderSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11DomainShader))
        *ppv = static_cast<ID3D11DomainShader*>(this);
    else if (riid == __uuidof(ID3D11DeviceChild))
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

Direct3D11DomainShaderSW::Direct3D11DomainShaderSW(ID3D11Device* device) : DeviceChildImpl(device) {}

}
