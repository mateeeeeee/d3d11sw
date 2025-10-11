#include "views/shader_resource_view.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE Direct3D11ShaderResourceViewSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11ShaderResourceView1))
        *ppv = static_cast<ID3D11ShaderResourceView1*>(this);
    else if (riid == __uuidof(ID3D11ShaderResourceView))
        *ppv = static_cast<ID3D11ShaderResourceView*>(this);
    else if (riid == __uuidof(ID3D11View))
        *ppv = static_cast<ID3D11View*>(this);
    else if (riid == __uuidof(ID3D11DeviceChild))
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

Direct3D11ShaderResourceViewSW::Direct3D11ShaderResourceViewSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11ShaderResourceViewSW::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
    {
        *ppResource = nullptr;
    }
}

void STDMETHODCALLTYPE Direct3D11ShaderResourceViewSW::GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
         *pDesc = {};
    }
}

void STDMETHODCALLTYPE Direct3D11ShaderResourceViewSW::GetDesc1(D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc)
{
    if (pDesc)
    {
         *pDesc = {};
    }
}

}
