#pragma once

#include "common/device_child_impl.h"

class MarsFence : public DeviceChildImpl<ID3D11Fence, ID3D11DeviceChild>
{
public:
    MarsFence(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE CreateSharedHandle(const SECURITY_ATTRIBUTES* pAttributes, DWORD dwAccess, LPCWSTR lpName, HANDLE* pHandle) override;
    UINT64 STDMETHODCALLTYPE GetCompletedValue() override;
    HRESULT STDMETHODCALLTYPE SetEventOnCompletion(UINT64 Value, HANDLE hEvent) override;
};
