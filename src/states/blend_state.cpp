#include "states/blend_state.h"

namespace d3d11sw {


Direct3D11BlendStateSW::Direct3D11BlendStateSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11BlendStateSW::GetDesc(D3D11_BLEND_DESC* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

void STDMETHODCALLTYPE Direct3D11BlendStateSW::GetDesc1(D3D11_BLEND_DESC1* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

}
