#include "shaders/hull_shader.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11HullShaderSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11HullShader))
    {
        *ppv = static_cast<ID3D11HullShader*>(this);
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

D3D11HullShaderSW::D3D11HullShaderSW(ID3D11Device* device) : DeviceChildImpl(device) {}

}
