#include "states/sampler_state.h"

namespace d3d11sw {


Direct3D11SamplerStateSW::Direct3D11SamplerStateSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE Direct3D11SamplerStateSW::GetDesc(D3D11_SAMPLER_DESC* pDesc)
{
    if (pDesc) 
    {
        *pDesc = {};
    }
}

}
