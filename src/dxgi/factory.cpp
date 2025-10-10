#include "dxgi/factory.h"

namespace d3d11sw {


DXGIFactorySW::DXGIFactorySW() {}
DXGIFactorySW::~DXGIFactorySW() {}


HRESULT STDMETHODCALLTYPE DXGIFactorySW::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::GetParent(REFIID riid, void** ppParent)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE DXGIFactorySW::EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::MakeWindowAssociation(HWND WindowHandle, UINT Flags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::GetWindowAssociation(HWND* pWindowHandle)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE DXGIFactorySW::EnumAdapters1(UINT Adapter, IDXGIAdapter1** ppAdapter)
{
    return E_NOTIMPL;
}

BOOL STDMETHODCALLTYPE DXGIFactorySW::IsCurrent()
{
    return TRUE;
}


BOOL STDMETHODCALLTYPE DXGIFactorySW::IsWindowedStereoEnabled()
{
    return FALSE;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::CreateSwapChainForHwnd(IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::CreateSwapChainForCoreWindow(IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::GetSharedResourceAdapterLuid(HANDLE hResource, LUID* pLuid)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::RegisterStereoStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::RegisterStereoStatusEvent(HANDLE hEvent, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE DXGIFactorySW::UnregisterStereoStatus(DWORD dwCookie)
{
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::RegisterOcclusionStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::RegisterOcclusionStatusEvent(HANDLE hEvent, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE DXGIFactorySW::UnregisterOcclusionStatus(DWORD dwCookie)
{
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::CreateSwapChainForComposition(IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
    return E_NOTIMPL;
}

}
