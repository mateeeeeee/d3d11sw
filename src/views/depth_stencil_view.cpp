#include "views/depth_stencil_view.h"

namespace d3d11sw {


Direct3D11DepthStencilViewSW::Direct3D11DepthStencilViewSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11DepthStencilViewSW::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
    {
        *ppResource = nullptr;
    }
}

void STDMETHODCALLTYPE Direct3D11DepthStencilViewSW::GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

}
