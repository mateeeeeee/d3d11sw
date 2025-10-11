#include "shaders/geometry_shader.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE Direct3D11GeometryShaderSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11GeometryShader))
        *ppv = static_cast<ID3D11GeometryShader*>(this);
    else if (riid == __uuidof(ID3D11DeviceChild))
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

Direct3D11GeometryShaderSW::Direct3D11GeometryShaderSW(ID3D11Device* device) : DeviceChildImpl(device) {}

}
