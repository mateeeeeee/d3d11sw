#pragma once
#include "common/unknown_impl.h"

namespace d3d11sw {


class DXGIFactorySW : public IDXGIFactory2, private UnknownBase
{
public:
    DXGIFactorySW();
    ~DXGIFactorySW();

    ULONG STDMETHODCALLTYPE AddRef() override  { return AddRefImpl(); }
    ULONG STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) final;

    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData) override;
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData) override;
    HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent) override;

    HRESULT STDMETHODCALLTYPE EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter) override;
    HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND WindowHandle, UINT Flags) override;
    HRESULT STDMETHODCALLTYPE GetWindowAssociation(HWND* pWindowHandle) override;
    HRESULT STDMETHODCALLTYPE CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) override;
    HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter) override;

    HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT Adapter, IDXGIAdapter1** ppAdapter) override;
    BOOL    STDMETHODCALLTYPE IsCurrent() override;

    BOOL    STDMETHODCALLTYPE IsWindowedStereoEnabled() override;
    HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) override;
    HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow(IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) override;
    HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid(HANDLE hResource, LUID* pLuid) override;
    HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie) override;
    HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent(HANDLE hEvent, DWORD* pdwCookie) override;
    void    STDMETHODCALLTYPE UnregisterStereoStatus(DWORD dwCookie) override;
    HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie) override;
    HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent(HANDLE hEvent, DWORD* pdwCookie) override;
    void    STDMETHODCALLTYPE UnregisterOcclusionStatus(DWORD dwCookie) override;
    HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition(IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain) override;
};

}
