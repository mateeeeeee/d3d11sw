#include "states/sampler_state.h"

MarsSamplerState::MarsSamplerState(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsSamplerState::GetDesc(D3D11_SAMPLER_DESC* pDesc)
{
    if (pDesc) *pDesc = {};
}
