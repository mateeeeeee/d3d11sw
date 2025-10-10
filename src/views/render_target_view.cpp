#include "views/render_target_view.h"

MarsRenderTargetView::MarsRenderTargetView(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsRenderTargetView::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
        *ppResource = nullptr;
}

void STDMETHODCALLTYPE MarsRenderTargetView::GetDesc(D3D11_RENDER_TARGET_VIEW_DESC* pDesc)
{
    if (pDesc)
        *pDesc = {};
}

void STDMETHODCALLTYPE MarsRenderTargetView::GetDesc1(D3D11_RENDER_TARGET_VIEW_DESC1* pDesc)
{
    if (pDesc)
        *pDesc = {};
}
