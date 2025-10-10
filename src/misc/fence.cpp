#include "misc/fence.h"

namespace d3d11sw {


Direct3D11FenceSW::Direct3D11FenceSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

HRESULT STDMETHODCALLTYPE Direct3D11FenceSW::CreateSharedHandle(const SECURITY_ATTRIBUTES* pAttributes, DWORD dwAccess, LPCWSTR lpName, HANDLE* pHandle)
{
    return E_NOTIMPL;
}

UINT64 STDMETHODCALLTYPE Direct3D11FenceSW::GetCompletedValue()
{
    return 0;
}

HRESULT STDMETHODCALLTYPE Direct3D11FenceSW::SetEventOnCompletion(UINT64 Value, HANDLE hEvent)
{
    return E_NOTIMPL;
}

}
