#include "views/unordered_access_view.h"

MarsUnorderedAccessView::MarsUnorderedAccessView(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsUnorderedAccessView::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
        *ppResource = nullptr;
}

void STDMETHODCALLTYPE MarsUnorderedAccessView::GetDesc(D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc)
{
    if (pDesc)
        *pDesc = {};
}

void STDMETHODCALLTYPE MarsUnorderedAccessView::GetDesc1(D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc)
{
    if (pDesc)
        *pDesc = {};
}
