#include "misc/input_layout.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11InputLayoutSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11InputLayout))
    {
        *ppv = static_cast<ID3D11InputLayout*>(this);
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

D3D11InputLayoutSW::D3D11InputLayoutSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

}
