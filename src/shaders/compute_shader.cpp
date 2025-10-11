#include "shaders/compute_shader.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE Direct3D11ComputeShaderSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11ComputeShader))
        *ppv = static_cast<ID3D11ComputeShader*>(this);
    else if (riid == __uuidof(ID3D11DeviceChild))
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

Direct3D11ComputeShaderSW::Direct3D11ComputeShaderSW(ID3D11Device* device) : DeviceChildImpl(device) {}

}
