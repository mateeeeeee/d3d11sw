#include "views/shader_resource_view.h"

namespace d3d11sw {


Direct3D11ShaderResourceViewSW::Direct3D11ShaderResourceViewSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11ShaderResourceViewSW::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
    {
        *ppResource = nullptr;
    }
}

void STDMETHODCALLTYPE Direct3D11ShaderResourceViewSW::GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
         *pDesc = {};
    }
}

void STDMETHODCALLTYPE Direct3D11ShaderResourceViewSW::GetDesc1(D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc)
{
    if (pDesc)
    {
         *pDesc = {};
    }
}

}
