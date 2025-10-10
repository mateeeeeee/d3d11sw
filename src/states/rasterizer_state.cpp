#include "states/rasterizer_state.h"

MarsRasterizerState::MarsRasterizerState(ID3D11Device* device)
    : DeviceChildImpl(device) {}

void STDMETHODCALLTYPE MarsRasterizerState::GetDesc(D3D11_RASTERIZER_DESC* pDesc)
{
    if (pDesc) *pDesc = {};
}

void STDMETHODCALLTYPE MarsRasterizerState::GetDesc1(D3D11_RASTERIZER_DESC1* pDesc)
{
    if (pDesc) *pDesc = {};
}

void STDMETHODCALLTYPE MarsRasterizerState::GetDesc2(D3D11_RASTERIZER_DESC2* pDesc)
{
    if (pDesc) *pDesc = {};
}
