#pragma once
#include "context2.h"

class MarsDeviceContext3 : public MarsDeviceContext2
{
public:
    explicit MarsDeviceContext3(ID3D11Device* device);

    void STDMETHODCALLTYPE Flush1(D3D11_CONTEXT_TYPE ContextType, HANDLE hEvent) override;
    void STDMETHODCALLTYPE SetHardwareProtectionState(BOOL HwProtectionEnable) override;
    void STDMETHODCALLTYPE GetHardwareProtectionState(BOOL* pHwProtectionEnable) override;
};
