#include "context3.h"

MarsDeviceContext3::MarsDeviceContext3(ID3D11Device* device)
    : MarsDeviceContext2(device)
{
}

void STDMETHODCALLTYPE MarsDeviceContext3::Flush1(D3D11_CONTEXT_TYPE ContextType, HANDLE hEvent)
{
}

void STDMETHODCALLTYPE MarsDeviceContext3::SetHardwareProtectionState(BOOL HwProtectionEnable)
{
}

void STDMETHODCALLTYPE MarsDeviceContext3::GetHardwareProtectionState(BOOL* pHwProtectionEnable)
{
}
