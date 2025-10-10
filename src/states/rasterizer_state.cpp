#include "states/rasterizer_state.h"

namespace d3d11sw {


Direct3D11RasterizerStateSW::Direct3D11RasterizerStateSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11RasterizerStateSW::GetDesc(D3D11_RASTERIZER_DESC* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE Direct3D11RasterizerStateSW::GetDesc1(D3D11_RASTERIZER_DESC1* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE Direct3D11RasterizerStateSW::GetDesc2(D3D11_RASTERIZER_DESC2* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

}
