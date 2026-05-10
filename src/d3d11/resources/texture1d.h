#pragma once
#include "d3d11/common/d3d11_headers.h"
#include <vector>
#include "core/format/subresource_layout.h"
#include "d3d11/device/device_child_impl.h"

namespace d3dsw {

class D3D11Texture1DSW final : public DeviceChildImpl<ID3D11Texture1D>
{
public:
    explicit D3D11Texture1DSW(ID3D11Device* device);

    HRESULT Init(const D3D11_TEXTURE1D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension) override;
    void STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) override {}
    UINT STDMETHODCALLTYPE GetEvictionPriority() override { return 0; }

    void STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE1D_DESC* pDesc) override;

    void*                       GetDataPtr()                                 { return _data.data(); }
    UINT64                      GetDataSize()                           const { return _data.size(); }
    D3D11_USAGE                 GetUsage()                              const { return _desc.Usage; }
    UINT                        GetCPUAccessFlags()                     const { return _desc.CPUAccessFlags; }
    UINT                        GetBindFlags()                          const { return _desc.BindFlags; }
    UINT                        GetSubresourceCount()                   const { return (UINT)_layouts.size(); }
    D3DSW_API D3DSW_SUBRESOURCE_LAYOUT GetSubresourceLayout(UINT Subresource) const;

private:
    std::vector<Uint8>                      _data;
    D3D11_TEXTURE1D_DESC                    _desc{};
    std::vector<D3DSW_SUBRESOURCE_LAYOUT> _layouts;
};

}
