#include "device5.h"

HRESULT STDMETHODCALLTYPE MarsDevice5::OpenSharedFence(
    HANDLE hFence,
    REFIID ReturnedInterface,
    void** ppFence)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice5::CreateFence(
    UINT64 InitialValue,
    D3D11_FENCE_FLAG Flags,
    REFIID ReturnedInterface,
    void** ppFence)
{
    return E_NOTIMPL;
}
