#include "views/render_target_view.h"

namespace d3d11sw {


Direct3D11RenderTargetViewSW::Direct3D11RenderTargetViewSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11RenderTargetViewSW::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
    {
        *ppResource = nullptr;
    }
}

void STDMETHODCALLTYPE Direct3D11RenderTargetViewSW::GetDesc(D3D11_RENDER_TARGET_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE Direct3D11RenderTargetViewSW::GetDesc1(D3D11_RENDER_TARGET_VIEW_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

}
