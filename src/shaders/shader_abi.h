#pragma once
#include "common/common.h"

namespace d3d11sw {

static constexpr Uint32 SW_MAX_VS_INPUTS  = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
static constexpr Uint32 SW_MAX_VARYINGS   = D3D11_VS_OUTPUT_REGISTER_COUNT;
static constexpr Uint32 SW_MAX_PS_OUTPUTS = D3D11_PS_OUTPUT_REGISTER_COUNT;
static constexpr Uint32 SW_MAX_CBUFS      = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
static constexpr Uint32 SW_MAX_TEXTURES   = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
static constexpr Uint32 SW_MAX_SAMPLERS   = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;

struct SW_float4
{
    Float x, y, z, w;
};

struct SW_uint3
{
    Uint32 x, y, z;
};

struct SW_Texture
{
    const void* data;
    Uint32      width;
    Uint32      height;
    Uint32      depth;
    Uint32      rowPitch;
    Uint32      slicePitch;
    Uint32      mipLevels;
    DXGI_FORMAT format;
};

struct SW_Sampler
{
    D3D11_FILTER               filter;
    D3D11_TEXTURE_ADDRESS_MODE addressU;
    D3D11_TEXTURE_ADDRESS_MODE addressV;
    D3D11_TEXTURE_ADDRESS_MODE addressW;
    Float                      mipLODBias;
    Float                      minLOD;
    Float                      maxLOD;
};

struct SW_Resources
{
    const SW_float4* cb[SW_MAX_CBUFS];
    SW_Texture       tex[SW_MAX_TEXTURES];
    SW_Sampler       smp[SW_MAX_SAMPLERS];
};

struct SW_VSInput
{
    SW_float4 v[SW_MAX_VS_INPUTS];
};

struct SW_VSOutput
{
    SW_float4 pos;
    SW_float4 o[SW_MAX_VARYINGS];
};

struct SW_PSInput
{
    SW_float4 pos;
    SW_float4 v[SW_MAX_VARYINGS];
};

struct SW_PSOutput
{
    SW_float4 oC[SW_MAX_PS_OUTPUTS];
    Float     oDepth;
    Bool      depthWritten;
};

struct SW_CSInput
{
    SW_uint3 groupID;
    SW_uint3 groupThreadID;
    SW_uint3 dispatchThreadID;
};

using SW_VSFn = void(*)(const SW_VSInput*, SW_VSOutput*, const SW_Resources*);
using SW_PSFn = void(*)(const SW_PSInput*, SW_PSOutput*, const SW_Resources*);
using SW_CSFn = void(*)(const SW_CSInput*, const SW_Resources*);

}
