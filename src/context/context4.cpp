#include "context4.h"

MarsDeviceContext4::MarsDeviceContext4(ID3D11Device* device)
    : MarsDeviceContext3(device)
{
}

HRESULT STDMETHODCALLTYPE MarsDeviceContext4::Signal(ID3D11Fence* pFence, UINT64 Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDeviceContext4::Wait(ID3D11Fence* pFence, UINT64 Value)
{
    return E_NOTIMPL;
}
