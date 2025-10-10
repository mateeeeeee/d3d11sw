#include "views/unordered_access_view.h"

namespace d3d11sw {


Direct3D11UnorderedAccessViewSW::Direct3D11UnorderedAccessViewSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11UnorderedAccessViewSW::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
    {
        *ppResource = nullptr;
    }
}

void STDMETHODCALLTYPE Direct3D11UnorderedAccessViewSW::GetDesc(D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE Direct3D11UnorderedAccessViewSW::GetDesc1(D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = {};
    }
}

}
