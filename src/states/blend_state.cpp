#include "states/blend_state.h"

MarsBlendState::MarsBlendState(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsBlendState::GetDesc(D3D11_BLEND_DESC* pDesc)
{
    if (pDesc) *pDesc = {};
}

void STDMETHODCALLTYPE MarsBlendState::GetDesc1(D3D11_BLEND_DESC1* pDesc)
{
    if (pDesc) *pDesc = {};
}
