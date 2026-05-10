#pragma once
#include "d3d11/common/d3d11_headers.h"
#include "d3d11/device/device_child_impl.h"
#include "core/format/subresource_layout.h"

namespace d3dsw {


class D3DSW_API D3D11RenderTargetViewSW final : public DeviceChildImpl<ID3D11RenderTargetView1>
{
public:
    explicit D3D11RenderTargetViewSW(ID3D11Device* device);
    ~D3D11RenderTargetViewSW() override;

    HRESULT Init(ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC1* pDesc);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    void STDMETHODCALLTYPE GetResource(ID3D11Resource** ppResource) override;
    void STDMETHODCALLTYPE GetDesc(D3D11_RENDER_TARGET_VIEW_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_RENDER_TARGET_VIEW_DESC1* pDesc) override;

    UINT8*                     GetDataPtr() const { return _dataPtr; }
    D3DSW_SUBRESOURCE_LAYOUT GetLayout()  const { return _layout; }
    DXGI_FORMAT                GetFormat()  const { return _desc.Format; }
    Uint                       GetSampleCount() const { return _sampleCount; }
    Uint                       GetSampleQuality() const { return _sampleQuality; }

private:
    ID3D11Resource*                _resource = nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC1 _desc{};
    UINT8*                         _dataPtr  = nullptr;
    D3DSW_SUBRESOURCE_LAYOUT     _layout{};
    Uint                           _sampleCount = 1;
    Uint                           _sampleQuality = 0;
};

}
