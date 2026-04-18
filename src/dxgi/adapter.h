#pragma once

#include "dxgi/dxgi_object_impl.h"

namespace d3d11sw {


class DXGIAdapterSW final: public DXGIObjectImpl<IDXGIAdapter1>
{
public:
    DXGIAdapterSW();
    ~DXGIAdapterSW();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent) override;

    HRESULT STDMETHODCALLTYPE EnumOutputs(UINT Output, IDXGIOutput** ppOutput) override;
    HRESULT STDMETHODCALLTYPE GetDesc(DXGI_ADAPTER_DESC* pDesc) override;
    HRESULT STDMETHODCALLTYPE CheckInterfaceSupport(REFGUID InterfaceName, LARGE_INTEGER* pUMDVersion) override;

    HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_ADAPTER_DESC1* pDesc) override;

private:
    IDXGIFactory* _factory = nullptr;
    IDXGIOutput*  _output  = nullptr;
};

}
