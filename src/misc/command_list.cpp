#include "misc/command_list.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE Direct3D11CommandListSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11CommandList))
        *ppv = static_cast<ID3D11CommandList*>(this);
    else if (riid == __uuidof(ID3D11DeviceChild))
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

Direct3D11CommandListSW::Direct3D11CommandListSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE Direct3D11CommandListSW::GetContextFlags()
{
    return 0;
}

}
