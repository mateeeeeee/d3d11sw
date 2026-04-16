#pragma once
#include "common/unknown_impl.h"

namespace d3d11sw {

class DXGIOutputSW final : public IDXGIOutput1, private UnknownBase
{
public:
    DXGIOutputSW() = default;
    ~DXGIOutputSW() override = default;

    ULONG STDMETHODCALLTYPE AddRef() override { return AddRefImpl(); }
    ULONG STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    // IDXGIObject
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent) override { return E_NOTIMPL; }

    // IDXGIOutput
    HRESULT STDMETHODCALLTYPE GetDesc(DXGI_OUTPUT_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE GetDisplayModeList(DXGI_FORMAT EnumFormat, UINT Flags, UINT* pNumModes, DXGI_MODE_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE FindClosestMatchingMode(const DXGI_MODE_DESC* pModeToMatch, DXGI_MODE_DESC* pClosestMatch, IUnknown* pConcernedDevice) override;
    HRESULT STDMETHODCALLTYPE WaitForVBlank() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE TakeOwnership(IUnknown* pDevice, BOOL Exclusive) override { return S_OK; }
    void    STDMETHODCALLTYPE ReleaseOwnership() override {}
    HRESULT STDMETHODCALLTYPE GetGammaControlCapabilities(DXGI_GAMMA_CONTROL_CAPABILITIES* pGammaCaps) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetGammaControl(const DXGI_GAMMA_CONTROL* pArray) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE GetGammaControl(DXGI_GAMMA_CONTROL* pArray) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetDisplaySurface(IDXGISurface* pScanoutSurface) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData(IDXGISurface* pDestination) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats) override { return E_NOTIMPL; }

    // IDXGIOutput1
    HRESULT STDMETHODCALLTYPE GetDisplayModeList1(DXGI_FORMAT EnumFormat, UINT Flags, UINT* pNumModes, DXGI_MODE_DESC1* pDesc) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE FindClosestMatchingMode1(const DXGI_MODE_DESC1* pModeToMatch, DXGI_MODE_DESC1* pClosestMatch, IUnknown* pConcernedDevice) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData1(IDXGIResource* pDestination) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE DuplicateOutput(IUnknown* pDevice, IDXGIOutputDuplication** ppOutputDuplication) override { return E_NOTIMPL; }
};

}
