#pragma once

#include "device4.h"

class MarsDevice5 : public MarsDevice4
{
public:
    HRESULT STDMETHODCALLTYPE OpenSharedFence(
        HANDLE hFence,
        REFIID ReturnedInterface,
        void** ppFence) override;

    HRESULT STDMETHODCALLTYPE CreateFence(
        UINT64 InitialValue,
        D3D11_FENCE_FLAG Flags,
        REFIID ReturnedInterface,
        void** ppFence) override;
};
