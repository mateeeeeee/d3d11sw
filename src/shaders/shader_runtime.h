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

// Bitcasts
inline unsigned sw_bits_uint(float f)  { unsigned u; std::memcpy(&u, &f, 4); return u; }
inline int      sw_bits_int(float f)   { int i;      std::memcpy(&i, &f, 4); return i; }
inline float    sw_uint_bits(unsigned u) { float f;  std::memcpy(&f, &u, 4); return f; }
inline float    sw_int_bits(int i)     { float f;    std::memcpy(&f, &i, 4); return f; }

// Type conversions
inline float sw_utof(float v)  { return (float)sw_bits_uint(v); }    
inline float sw_itof(float v)  { return (float)sw_bits_int(v);  }    
inline float sw_ftou(float v)  { return sw_uint_bits((unsigned)v); } 
inline float sw_ftoi(float v)  { return sw_int_bits((int)v);     }   

// Integer arithmetic
inline float sw_iadd(float a, float b)  { return sw_int_bits(sw_bits_int(a)  +  sw_bits_int(b)); }
inline float sw_ishl(float a, float b)  { return sw_int_bits(sw_bits_int(a)  << (sw_bits_int(b) & 31)); }
inline float sw_ishr(float a, float b)  { return sw_int_bits(sw_bits_int(a)  >> (sw_bits_int(b) & 31)); }
inline float sw_ushr(float a, float b)  { return sw_uint_bits(sw_bits_uint(a) >> (sw_bits_uint(b) & 31)); }
inline float sw_ineg(float a)           { return sw_int_bits(-sw_bits_int(a)); }
inline float sw_imax(float a, float b)  { int ia = sw_bits_int(a), ib = sw_bits_int(b); return sw_int_bits(ia > ib ? ia : ib); }
inline float sw_imin(float a, float b)  { int ia = sw_bits_int(a), ib = sw_bits_int(b); return sw_int_bits(ia < ib ? ia : ib); }
inline float sw_umax(float a, float b)  { unsigned ua = sw_bits_uint(a), ub = sw_bits_uint(b); return sw_uint_bits(ua > ub ? ua : ub); }
inline float sw_umin(float a, float b)  { unsigned ua = sw_bits_uint(a), ub = sw_bits_uint(b); return sw_uint_bits(ua < ub ? ua : ub); }
inline float sw_and(float a, float b)   { return sw_uint_bits(sw_bits_uint(a) & sw_bits_uint(b)); }
inline float sw_or(float a, float b)    { return sw_uint_bits(sw_bits_uint(a) | sw_bits_uint(b)); }
inline float sw_xor(float a, float b)   { return sw_uint_bits(sw_bits_uint(a) ^ sw_bits_uint(b)); }
inline float sw_not(float a)            { return sw_uint_bits(~sw_bits_uint(a)); }

// float4 intrinsics
inline SW_float4 sw_abs4(SW_float4 v)
{
    return { sw_abs(v.x), sw_abs(v.y), sw_abs(v.z), sw_abs(v.w) };
}

inline SW_float4 sw_ineg4(SW_float4 v)
{
    return { sw_ineg(v.x), sw_ineg(v.y), sw_ineg(v.z), sw_ineg(v.w) };
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

static inline SW_float4 sw_uav_load_typed(const SW_UAV& u, unsigned idx)
{
    if (!u.data || idx >= u.elementCount) { return {0,0,0,0}; }
    switch (u.format)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        {
            float v[4];
            std::memcpy(v, static_cast<const unsigned char*>(u.data) + idx * 16u, 16);
            return { v[0], v[1], v[2], v[3] };
        }
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        {
            unsigned v[4];
            std::memcpy(v, static_cast<const unsigned char*>(u.data) + idx * 16u, 16);
            return { (float)v[0], (float)v[1], (float)v[2], (float)v[3] };
        }
        case DXGI_FORMAT_R32_FLOAT:
        {
            float v;
            std::memcpy(&v, static_cast<const unsigned char*>(u.data) + idx * 4u, 4);
            return { v, 0, 0, 0 };
        }
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        {
            unsigned v;
            std::memcpy(&v, static_cast<const unsigned char*>(u.data) + idx * 4u, 4);
            return { (float)v, 0, 0, 0 };
        }
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        {
            const unsigned char* p = static_cast<const unsigned char*>(u.data) + idx * 4u;
            return { p[0]/255.f, p[1]/255.f, p[2]/255.f, p[3]/255.f };
        }
        default: return {0,0,0,0};
    }
}

static inline void sw_uav_store_typed(SW_UAV& u, unsigned idx, SW_float4 val)
{
    if (!u.data || idx >= u.elementCount) { return; }
    switch (u.format)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        {
            float v[4] = { val.x, val.y, val.z, val.w };
            std::memcpy(static_cast<unsigned char*>(u.data) + idx * 16u, v, 16);
            break;
        }
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        {
            unsigned v[4] = { (unsigned)val.x, (unsigned)val.y, (unsigned)val.z, (unsigned)val.w };
            std::memcpy(static_cast<unsigned char*>(u.data) + idx * 16u, v, 16);
            break;
        }
        case DXGI_FORMAT_R32_FLOAT:
        {
            std::memcpy(static_cast<unsigned char*>(u.data) + idx * 4u, &val.x, 4);
            break;
        }
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        {
            unsigned v = (unsigned)val.x;
            std::memcpy(static_cast<unsigned char*>(u.data) + idx * 4u, &v, 4);
            break;
        }
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        {
            unsigned char* p = static_cast<unsigned char*>(u.data) + idx * 4u;
            p[0] = (unsigned char)sw_saturate(val.x) * 255;
            p[1] = (unsigned char)sw_saturate(val.y) * 255;
            p[2] = (unsigned char)sw_saturate(val.z) * 255;
            p[3] = (unsigned char)sw_saturate(val.w) * 255;
            break;
        }
        default: break;
    }
}

// UAV raw load/store (byte address buffer, R32_TYPELESS)
static inline SW_float4 sw_uav_load_raw(const SW_UAV& u, unsigned byteOffset)
{
    if (!u.data) { return {0,0,0,0}; }
    unsigned v;
    std::memcpy(&v, static_cast<const unsigned char*>(u.data) + byteOffset, 4);
    return { (float)v, 0, 0, 0 };
}

static inline void sw_uav_store_raw(SW_UAV& u, unsigned byteOffset, SW_float4 val)
{
    if (!u.data) { return; }
    unsigned v = (unsigned)val.x;
    std::memcpy(static_cast<unsigned char*>(u.data) + byteOffset, &v, 4);
}

static inline SW_float4 sw_uav_load_structured(const SW_UAV& u, unsigned idx, unsigned byteOffset)
{
    if (!u.data || !u.stride) { return {0,0,0,0}; }
    unsigned base = idx * u.stride + byteOffset;
    unsigned v;
    std::memcpy(&v, static_cast<const unsigned char*>(u.data) + base, 4);
    return { (float)v, 0, 0, 0 };
}

static inline void sw_uav_store_structured(SW_UAV& u, unsigned idx, unsigned byteOffset, SW_float4 val)
{
    if (!u.data || !u.stride) { return; }
    unsigned base = idx * u.stride + byteOffset;
    unsigned v = (unsigned)val.x;
    std::memcpy(static_cast<unsigned char*>(u.data) + base, &v, 4);
}

// TGSM load/store
static inline SW_float4 sw_tgsm_load_raw(SW_TGSM& g, unsigned byteOffset)
{
    if (!g.data) { return {0,0,0,0}; }
    unsigned v;
    std::memcpy(&v, static_cast<const unsigned char*>(g.data) + byteOffset, 4);
    return { sw_uint_bits(v), 0, 0, 0 };
}

static inline void sw_tgsm_store_raw(SW_TGSM& g, unsigned byteOffset, SW_float4 val)
{
    if (!g.data) { return; }
    unsigned v = sw_bits_uint(val.x);
    std::memcpy(static_cast<unsigned char*>(g.data) + byteOffset, &v, 4);
}

static inline SW_float4 sw_tgsm_load_structured(SW_TGSM& g, unsigned idx, unsigned byteOffset)
{
    if (!g.data || !g.stride) { return {0,0,0,0}; }
    unsigned base = idx * g.stride + byteOffset;
    unsigned v;
    std::memcpy(&v, static_cast<const unsigned char*>(g.data) + base, 4);
    return { sw_uint_bits(v), 0, 0, 0 };
}

static inline void sw_tgsm_store_structured(SW_TGSM& g, unsigned idx, unsigned byteOffset, SW_float4 val)
{
    if (!g.data || !g.stride) { return; }
    unsigned base = idx * g.stride + byteOffset;
    unsigned v = sw_bits_uint(val.x);
    std::memcpy(static_cast<unsigned char*>(g.data) + base, &v, 4);
}
