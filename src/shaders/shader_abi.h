#pragma once
#include "common/common.h"

namespace d3d11sw {

static constexpr unsigned SW_MAX_VS_INPUTS  = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
static constexpr unsigned SW_MAX_VARYINGS   = D3D11_VS_OUTPUT_REGISTER_COUNT;
static constexpr unsigned SW_MAX_PS_OUTPUTS = D3D11_PS_OUTPUT_REGISTER_COUNT;
static constexpr unsigned SW_MAX_CBUFS      = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
static constexpr unsigned SW_MAX_TEXTURES   = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
static constexpr unsigned SW_MAX_SAMPLERS   = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
static constexpr unsigned SW_MAX_UAVS       = D3D11_1_UAV_SLOT_COUNT;
static constexpr unsigned SW_MAX_TGSM       = 8;

struct SW_float4
{
    float x, y, z, w;
};

struct SW_uint3
{
    unsigned x, y, z;
};

struct SW_SRV
{
    const void* data;
    unsigned    width;
    unsigned    height;
    unsigned    depth;
    unsigned    rowPitch;
    unsigned    slicePitch;
    unsigned    mipLevels;
    unsigned    stride;
    DXGI_FORMAT format;
};

struct SW_Sampler
{
    D3D11_FILTER               filter;
    D3D11_TEXTURE_ADDRESS_MODE addressU;
    D3D11_TEXTURE_ADDRESS_MODE addressV;
    D3D11_TEXTURE_ADDRESS_MODE addressW;
    float                      mipLODBias;
    float                      minLOD;
    float                      maxLOD;
    D3D11_COMPARISON_FUNC      comparisonFunc;
    float                      borderColor[4];
};

struct SW_UAV
{
    void*               data;
    unsigned            elementCount;
    unsigned            stride;        
    unsigned            width;
    unsigned            height;
    unsigned            depth;
    unsigned            rowPitch;
    unsigned            slicePitch;
    DXGI_FORMAT         format;
    D3D11_UAV_DIMENSION dimension;
};

struct SW_Resources
{
    const SW_float4* cb[SW_MAX_CBUFS];
    SW_SRV           srv[SW_MAX_TEXTURES];
    SW_Sampler       smp[SW_MAX_SAMPLERS];
    SW_UAV           uav[SW_MAX_UAVS];
};

struct SW_VSInput  { SW_float4 v[SW_MAX_VS_INPUTS]; };
struct SW_VSOutput { SW_float4 pos; SW_float4 o[SW_MAX_VARYINGS]; float clipDist[8]; float cullDist[8]; };

struct SW_PSInput  { SW_float4 pos; SW_float4 v[SW_MAX_VARYINGS]; unsigned isFrontFace; };
struct SW_PSOutput { SW_float4 oC[SW_MAX_PS_OUTPUTS]; float oDepth; bool depthWritten; bool discarded; };

struct SW_CSInput
{
    SW_uint3 groupID;
    SW_uint3 groupThreadID;
    SW_uint3 dispatchThreadID;
    unsigned groupIndex;
};

using SW_VSFn = void(*)(const SW_VSInput*, SW_VSOutput*, const SW_Resources*);
using SW_PSFn = void(*)(const SW_PSInput*, SW_PSOutput*, const SW_Resources*);

struct SW_TGSM
{
    void*    data;
    unsigned size;    
    unsigned stride;  
};

using SW_BarrierFn = void(*)(void*);
using SW_CSFn = void(*)(const SW_CSInput*, SW_Resources*, SW_TGSM*, SW_BarrierFn, void*);

}
