#pragma once
#include "shaders/shader_constants.h"

namespace d3d11sw {

struct SW_float4
{
    float x, y, z, w;
};

struct SW_uint3
{
    unsigned x, y, z;
};

struct SW_MipInfo
{
    unsigned width;
    unsigned height;
    unsigned depth;
    unsigned rowPitch;
    unsigned slicePitch;
};

struct SW_SRV
{
    const void* data;
    unsigned    mipLevels;
    unsigned    stride;
    unsigned    format;
    SW_MipInfo  mips[SW_MAX_MIP_LEVELS];
    unsigned    mipOffsets[SW_MAX_MIP_LEVELS];
};

struct SW_Sampler
{
    unsigned filter;
    unsigned addressU;
    unsigned addressV;
    unsigned addressW;
    float    mipLODBias;
    float    minLOD;
    float    maxLOD;
    unsigned comparisonFunc;
    float    borderColor[4];
    unsigned maxAnisotropy;
};

struct SW_UAV
{
    void*    data;
    unsigned elementCount;
    unsigned stride;
    unsigned width;
    unsigned height;
    unsigned depth;
    unsigned rowPitch;
    unsigned slicePitch;
    unsigned format;
    unsigned dimension;
    unsigned* counter;
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

struct SW_PSQuadInput
{
    SW_PSInput pixels[4];
    unsigned   activeMask;
};

struct SW_PSQuadOutput
{
    SW_PSOutput pixels[4];
};

using SW_PSQuadFn = void(*)(const SW_PSQuadInput*, SW_PSQuadOutput*, const SW_Resources*);

struct SW_TGSM
{
    void*    data;
    unsigned size;
    unsigned stride;
};

using SW_BarrierFn = void(*)(void*);
using SW_CSFn = void(*)(const SW_CSInput*, SW_Resources*, SW_TGSM*, SW_BarrierFn, void*);

}
