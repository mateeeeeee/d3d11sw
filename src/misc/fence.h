#pragma once

#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11FenceSW : public DeviceChildImpl<ID3D11Fence>
{
public:
    explicit Direct3D11FenceSW(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) final;

    HRESULT STDMETHODCALLTYPE CreateSharedHandle(const SECURITY_ATTRIBUTES* pAttributes, DWORD dwAccess, LPCWSTR lpName, HANDLE* pHandle) override;
    UINT64 STDMETHODCALLTYPE GetCompletedValue() override;
    HRESULT STDMETHODCALLTYPE SetEventOnCompletion(UINT64 Value, HANDLE hEvent) override;
};

}
