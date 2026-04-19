#pragma once

#include "device/device_child_impl.h"

namespace d3d11sw {


class D3D11FenceSW final : public DeviceChildImpl<ID3D11Fence>
{
public:
    explicit D3D11FenceSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE CreateSharedHandle(const SECURITY_ATTRIBUTES* pAttributes, DWORD dwAccess, LPCWSTR lpName, HANDLE* pHandle) override;
    UINT64 STDMETHODCALLTYPE GetCompletedValue() override;
    HRESULT STDMETHODCALLTYPE SetEventOnCompletion(UINT64 Value, HANDLE hEvent) override;
};

}
