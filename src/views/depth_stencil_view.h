#pragma once
#include "device/device_child_impl.h"
#include "resources/subresource_layout.h"

namespace d3d11sw {


class D3D11SW_API D3D11DepthStencilViewSW final : public DeviceChildImpl<ID3D11DepthStencilView>
{
public:
    explicit D3D11DepthStencilViewSW(ID3D11Device* device);
    ~D3D11DepthStencilViewSW() override;

    HRESULT Init(ID3D11Resource* pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    void STDMETHODCALLTYPE GetResource(ID3D11Resource** ppResource) override;
    void STDMETHODCALLTYPE GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc) override;

    UINT8*                     GetDataPtr() const { return _dataPtr; }
    D3D11SW_SUBRESOURCE_LAYOUT GetLayout()  const { return _layout; }
    DXGI_FORMAT                GetFormat()  const { return _desc.Format; }
    Uint                       GetSampleCount() const { return _sampleCount; }
    Uint                       GetSampleQuality() const { return _sampleQuality; }

private:
    ID3D11Resource*               _resource = nullptr;
    D3D11_DEPTH_STENCIL_VIEW_DESC _desc{};
    UINT8*                        _dataPtr  = nullptr;
    D3D11SW_SUBRESOURCE_LAYOUT    _layout{};
    Uint                          _sampleCount = 1;
    Uint                          _sampleQuality = 0;
};

}
