#pragma once

#include "device3.h"

class MarsDevice4 : public MarsDevice3
{
public:
    HRESULT STDMETHODCALLTYPE RegisterDeviceRemovedEvent(
        HANDLE hEvent,
        DWORD* pdwCookie) override;

    void STDMETHODCALLTYPE UnregisterDeviceRemoved(
        DWORD dwCookie) override;
};
