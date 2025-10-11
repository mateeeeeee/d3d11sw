#include "shaders/pixel_shader.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE Direct3D11PixelShaderSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11PixelShader))
        *ppv = static_cast<ID3D11PixelShader*>(this);
    else if (riid == __uuidof(ID3D11DeviceChild))
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

Direct3D11PixelShaderSW::Direct3D11PixelShaderSW(ID3D11Device* device) : DeviceChildImpl(device) {}

}
