#include "states/depth_stencil_state.h"

namespace d3d11sw {


Direct3D11DepthStencilStateSW::Direct3D11DepthStencilStateSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11DepthStencilStateSW::GetDesc(D3D11_DEPTH_STENCIL_DESC* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

}
