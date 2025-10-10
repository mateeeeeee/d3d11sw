#include "misc/fence.h"

MarsFence::MarsFence(ID3D11Device* device)
    : DeviceChildImpl(device) {}

HRESULT STDMETHODCALLTYPE MarsFence::CreateSharedHandle(const SECURITY_ATTRIBUTES* pAttributes, DWORD dwAccess, LPCWSTR lpName, HANDLE* pHandle)
{
    return E_NOTIMPL;
}

UINT64 STDMETHODCALLTYPE MarsFence::GetCompletedValue()
{
    return 0;
}

HRESULT STDMETHODCALLTYPE MarsFence::SetEventOnCompletion(UINT64 Value, HANDLE hEvent)
{
    return E_NOTIMPL;
}
