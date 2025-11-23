// File included at top for every generated C++ shader
#pragma once
#include "shaders/shader_abi.h"
#include <cmath>
#include <cstring>
#include <algorithm>

using namespace d3d11sw;

// float4 operators
inline SW_float4 operator+(SW_float4 a, SW_float4 b) { return {a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w}; }
inline SW_float4 operator-(SW_float4 a, SW_float4 b) { return {a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w}; }
inline SW_float4 operator*(SW_float4 a, SW_float4 b) { return {a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w}; }
inline SW_float4 operator/(SW_float4 a, SW_float4 b) { return {a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w}; }
inline SW_float4 operator-(SW_float4 a)              { return {-a.x, -a.y, -a.z, -a.w}; }

// Scalar intrinsics
inline float sw_dot2(SW_float4 a, SW_float4 b) { return a.x*b.x + a.y*b.y; }
inline float sw_dot3(SW_float4 a, SW_float4 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline float sw_dot4(SW_float4 a, SW_float4 b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

inline float sw_saturate(float v)     { return std::fmin(std::fmax(v, 0.f), 1.f); }
inline float sw_sqrt(float v)         { return std::sqrt(v); }
inline float sw_rsq(float v)          { return 1.f / std::sqrt(v); }
inline float sw_log2(float v)         { return std::log2(v); }
inline float sw_exp2(float v)         { return std::exp2(v); }
inline float sw_frc(float v)          { return v - std::floor(v); }
inline float sw_round_ne(float v)     { return std::nearbyint(v); }
inline float sw_round_z(float v)      { return std::trunc(v); }
inline float sw_min(float a, float b) { return std::fmin(a, b); }
inline float sw_max(float a, float b) { return std::fmax(a, b); }
inline float sw_abs(float v)          { return std::fabs(v); }
inline int   sw_ftoi(float v)         { return (int)v; }
inline float sw_itof(int v)           { return (float)v; }
inline float sw_utof(unsigned v)      { return (float)v; }
inline int   sw_ftou(float v)         { return (int)(unsigned)v; }

// float4 intrinsics
inline SW_float4 sw_abs4(SW_float4 v)
{
    return { sw_abs(v.x), sw_abs(v.y), sw_abs(v.z), sw_abs(v.w) };
}

inline SW_float4 sw_saturate4(SW_float4 v)
{
    return { sw_saturate(v.x), sw_saturate(v.y), sw_saturate(v.z), sw_saturate(v.w) };
}

inline SW_float4 sw_min4(SW_float4 a, SW_float4 b)
{
    return { sw_min(a.x,b.x), sw_min(a.y,b.y), sw_min(a.z,b.z), sw_min(a.w,b.w) };
}

inline SW_float4 sw_max4(SW_float4 a, SW_float4 b)
{
    return { sw_max(a.x,b.x), sw_max(a.y,b.y), sw_max(a.z,b.z), sw_max(a.w,b.w) };
}

inline SW_float4 sw_swizzle(SW_float4 v, int x, int y, int z, int w)
{
    const float* c = &v.x;
    return { c[x], c[y], c[z], c[w] };
}

// Texture sampling (2D, MVP: nearest + bilinear, WRAP and CLAMP for now)
static inline float sw_addr(float u, D3D11_TEXTURE_ADDRESS_MODE mode)
{
    if (mode == D3D11_TEXTURE_ADDRESS_CLAMP)
    {
        return std::fmin(std::fmax(u, 0.f), 1.f);
    }
    return u - std::floor(u); 
}

static inline SW_float4 sw_fetch_texel(const SW_Texture& t, unsigned x, unsigned y)
{
    x = std::min(x, t.width  - 1);
    y = std::min(y, t.height - 1);
    switch (t.format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        {
            const unsigned char* p = static_cast<const unsigned char*>(t.data)
                                   + (unsigned long long)y * t.rowPitch + x * 4u;
            return { p[0]/255.f, p[1]/255.f, p[2]/255.f, p[3]/255.f };
        }
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        {
            float v[4];
            std::memcpy(v, static_cast<const unsigned char*>(t.data)
                         + (unsigned long long)y * t.rowPitch + x * 16u, 16);
            return { v[0], v[1], v[2], v[3] };
        }
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        {
            const unsigned char* p = static_cast<const unsigned char*>(t.data)
                                   + (unsigned long long)y * t.rowPitch + x * 4u;
            return { p[2]/255.f, p[1]/255.f, p[0]/255.f, p[3]/255.f };
        }
        default:
            return { 0.f, 0.f, 0.f, 0.f };
    }
}

static inline SW_float4 sw_sample_2d(const SW_Texture& t, const SW_Sampler& s,
                                      float u, float v)
{
    float su = sw_addr(u, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressU));
    float sv = sw_addr(v, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressV));

    float fx = su * (float)t.width  - 0.5f;
    float fy = sv * (float)t.height - 0.5f;

    if (s.filter == D3D11_FILTER_MIN_MAG_MIP_POINT ||
        s.filter == D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR)
    {
        unsigned px = (unsigned)std::max(fx + 0.5f, 0.f);
        unsigned py = (unsigned)std::max(fy + 0.5f, 0.f);
        return sw_fetch_texel(t, px, py);
    }

    int   x0 = (int)std::floor(fx);
    int   y0 = (int)std::floor(fy);
    float tx  = fx - (float)x0;
    float ty  = fy - (float)y0;

    SW_float4 s00 = sw_fetch_texel(t, (unsigned)std::max(x0,   0), (unsigned)std::max(y0,   0));
    SW_float4 s10 = sw_fetch_texel(t, (unsigned)std::max(x0+1, 0), (unsigned)std::max(y0,   0));
    SW_float4 s01 = sw_fetch_texel(t, (unsigned)std::max(x0,   0), (unsigned)std::max(y0+1, 0));
    SW_float4 s11 = sw_fetch_texel(t, (unsigned)std::max(x0+1, 0), (unsigned)std::max(y0+1, 0));

    auto lerp4 = [](SW_float4 a, SW_float4 b, float t) -> SW_float4 {
        return { a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t, a.w+(b.w-a.w)*t };
    };

    return lerp4(lerp4(s00, s10, tx), lerp4(s01, s11, tx), ty);
}
