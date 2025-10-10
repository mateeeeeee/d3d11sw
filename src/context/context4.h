#pragma once
#include "context3.h"

class MarsDeviceContext4 : public MarsDeviceContext3
{
public:
    explicit MarsDeviceContext4(ID3D11Device* device);

    HRESULT STDMETHODCALLTYPE Signal(ID3D11Fence* pFence, UINT64 Value) override;
    HRESULT STDMETHODCALLTYPE Wait(ID3D11Fence* pFence, UINT64 Value) override;
};
