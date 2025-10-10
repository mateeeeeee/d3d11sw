#include "views/depth_stencil_view.h"

MarsDepthStencilView::MarsDepthStencilView(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsDepthStencilView::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
        *ppResource = nullptr;
}

void STDMETHODCALLTYPE MarsDepthStencilView::GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc)
{
    if (pDesc)
        *pDesc = {};
}
