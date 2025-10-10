#include "states/depth_stencil_state.h"

MarsDepthStencilState::MarsDepthStencilState(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsDepthStencilState::GetDesc(D3D11_DEPTH_STENCIL_DESC* pDesc)
{
    if (pDesc) *pDesc = {};
}
