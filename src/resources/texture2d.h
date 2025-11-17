#pragma once
#include <vector>
#include "subresource_layout.h"
#include "common/device_child_impl.h"

namespace d3d11sw {

class D3D11Texture2DSW final : public DeviceChildImpl<ID3D11Texture2D1>
{
public:
    explicit D3D11Texture2DSW(ID3D11Device* device);

    HRESULT Init(const D3D11_TEXTURE2D_DESC1* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension) override;
    void STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) override {}
    UINT STDMETHODCALLTYPE GetEvictionPriority() override { return 0; }

    void STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE2D_DESC* pDesc) override;
    void STDMETHODCALLTYPE GetDesc1(D3D11_TEXTURE2D_DESC1* pDesc) override;

    void*                       GetDataPtr()                                 { return _data.data(); }
    UINT64                      GetDataSize()                           const { return _data.size(); }
    D3D11_USAGE                 GetUsage()                              const { return _desc.Usage; }
    UINT                        GetSubresourceCount()                   const { return (UINT)_layouts.size(); }
    D3D11SW_SUBRESOURCE_LAYOUT  GetSubresourceLayout(UINT Subresource)  const;

private:
    std::vector<Uint8>                      _data;
    D3D11_TEXTURE2D_DESC1                   _desc{};
    std::vector<D3D11SW_SUBRESOURCE_LAYOUT> _layouts;
};

}
