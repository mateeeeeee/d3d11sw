#include "views/shader_resource_view.h"

MarsShaderResourceView::MarsShaderResourceView(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsShaderResourceView::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
        *ppResource = nullptr;
}

void STDMETHODCALLTYPE MarsShaderResourceView::GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc)
{
    if (pDesc)
        *pDesc = {};
}

void STDMETHODCALLTYPE MarsShaderResourceView::GetDesc1(D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc)
{
    if (pDesc)
        *pDesc = {};
}
