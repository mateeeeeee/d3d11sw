#include "device4.h"

HRESULT STDMETHODCALLTYPE MarsDevice4::RegisterDeviceRemovedEvent(
    HANDLE hEvent,
    DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE MarsDevice4::UnregisterDeviceRemoved(
    DWORD dwCookie)
{
}
