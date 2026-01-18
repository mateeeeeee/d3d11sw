// File included at top for every generated C++ shader
#pragma once
#include "shaders/shader_abi.h"
#include <atomic>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <limits>

#ifdef _WIN32
#define SW_JIT_EXPORT __declspec(dllexport)
#else
#define SW_JIT_EXPORT
#endif

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
inline float sw_round_ni(float v)     { return std::floor(v); }
inline float sw_round_pi(float v)     { return std::ceil(v); }
inline float sw_sin(float v)          { return std::sin(v); }
inline float sw_cos(float v)          { return std::cos(v); }
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
inline float sw_ieq(float a, float b)  { return sw_uint_bits(sw_bits_int(a) == sw_bits_int(b) ? 0xFFFFFFFFu : 0u); }
inline float sw_ine(float a, float b)  { return sw_uint_bits(sw_bits_int(a) != sw_bits_int(b) ? 0xFFFFFFFFu : 0u); }
inline float sw_ige(float a, float b)  { return sw_uint_bits(sw_bits_int(a) >= sw_bits_int(b) ? 0xFFFFFFFFu : 0u); }
inline float sw_ilt(float a, float b)  { return sw_uint_bits(sw_bits_int(a) <  sw_bits_int(b) ? 0xFFFFFFFFu : 0u); }
inline float sw_uge(float a, float b)  { return sw_uint_bits(sw_bits_uint(a) >= sw_bits_uint(b) ? 0xFFFFFFFFu : 0u); }
inline float sw_ult(float a, float b)  { return sw_uint_bits(sw_bits_uint(a) <  sw_bits_uint(b) ? 0xFFFFFFFFu : 0u); }
inline float sw_udiv(float a, float b) { unsigned ub = sw_bits_uint(b); return ub ? sw_uint_bits(sw_bits_uint(a) / ub) : sw_uint_bits(0xFFFFFFFFu); }
inline float sw_umod(float a, float b) { unsigned ub = sw_bits_uint(b); return ub ? sw_uint_bits(sw_bits_uint(a) % ub) : 0.f; }
inline float sw_imul_hi(float a, float b) { long long r = (long long)sw_bits_int(a) * (long long)sw_bits_int(b); return sw_int_bits((int)(r >> 32)); }
inline float sw_imul_lo(float a, float b) { return sw_int_bits(sw_bits_int(a) * sw_bits_int(b)); }
inline float sw_umul_hi(float a, float b) { unsigned long long r = (unsigned long long)sw_bits_uint(a) * (unsigned long long)sw_bits_uint(b); return sw_uint_bits((unsigned)(r >> 32)); }
inline float sw_umul_lo(float a, float b) { return sw_uint_bits(sw_bits_uint(a) * sw_bits_uint(b)); }
inline float sw_umad(float a, float b, float c) { return sw_uint_bits(sw_bits_uint(a) * sw_bits_uint(b) + sw_bits_uint(c)); }
inline float sw_uaddc(float a, float b, float& carry) { unsigned ua = sw_bits_uint(a), ub = sw_bits_uint(b); unsigned r = ua + ub; carry = sw_uint_bits(r < ua ? 1u : 0u); return sw_uint_bits(r); }
inline float sw_usubb(float a, float b, float& borrow) { unsigned ua = sw_bits_uint(a), ub = sw_bits_uint(b); borrow = sw_uint_bits(ua < ub ? 1u : 0u); return sw_uint_bits(ua - ub); }
inline float sw_feq(float a, float b) { return sw_uint_bits(a == b ? 0xFFFFFFFFu : 0u); }
inline float sw_fne(float a, float b) { return sw_uint_bits(a != b ? 0xFFFFFFFFu : 0u); }
inline float sw_fge(float a, float b) { return sw_uint_bits(a >= b ? 0xFFFFFFFFu : 0u); }
inline float sw_flt(float a, float b) { return sw_uint_bits(a <  b ? 0xFFFFFFFFu : 0u); }
inline float sw_and(float a, float b)   { return sw_uint_bits(sw_bits_uint(a) & sw_bits_uint(b)); }
inline float sw_or(float a, float b)    { return sw_uint_bits(sw_bits_uint(a) | sw_bits_uint(b)); }
inline float sw_xor(float a, float b)   { return sw_uint_bits(sw_bits_uint(a) ^ sw_bits_uint(b)); }
inline float sw_not(float a)            { return sw_uint_bits(~sw_bits_uint(a)); }
inline float sw_imad(float a, float b, float c) { return sw_int_bits(sw_bits_int(a) * sw_bits_int(b) + sw_bits_int(c)); }
inline float sw_countbits(float a)     { unsigned v = sw_bits_uint(a); v = v - ((v >> 1) & 0x55555555u); v = (v & 0x33333333u) + ((v >> 2) & 0x33333333u); return sw_uint_bits(((v + (v >> 4)) & 0x0F0F0F0Fu) * 0x01010101u >> 24); }
inline float sw_firstbit_hi(float a)   { unsigned v = sw_bits_uint(a); if (!v) { return sw_uint_bits(0xFFFFFFFFu); } unsigned n = 0; if (!(v & 0xFFFF0000u)) { n += 16; v <<= 16; } if (!(v & 0xFF000000u)) { n += 8; v <<= 8; } if (!(v & 0xF0000000u)) { n += 4; v <<= 4; } if (!(v & 0xC0000000u)) { n += 2; v <<= 2; } if (!(v & 0x80000000u)) { n += 1; } return sw_uint_bits(n); }
inline float sw_firstbit_lo(float a)   { unsigned v = sw_bits_uint(a); if (!v) { return sw_uint_bits(0xFFFFFFFFu); } unsigned n = 0; if (!(v & 0xFFFFu)) { n += 16; v >>= 16; } if (!(v & 0xFFu)) { n += 8; v >>= 8; } if (!(v & 0xFu)) { n += 4; v >>= 4; } if (!(v & 0x3u)) { n += 2; v >>= 2; } if (!(v & 0x1u)) { n += 1; } return sw_uint_bits(n); }
inline float sw_firstbit_shi(float a)  { int v = sw_bits_int(a); if (v == 0 || v == -1) { return sw_uint_bits(0xFFFFFFFFu); } if (v < 0) { v = ~v; } return sw_firstbit_hi(sw_uint_bits((unsigned)v)); }
inline float sw_bfrev(float a)         { unsigned v = sw_bits_uint(a); v = ((v >> 1) & 0x55555555u) | ((v & 0x55555555u) << 1); v = ((v >> 2) & 0x33333333u) | ((v & 0x33333333u) << 2); v = ((v >> 4) & 0x0F0F0F0Fu) | ((v & 0x0F0F0F0Fu) << 4); v = ((v >> 8) & 0x00FF00FFu) | ((v & 0x00FF00FFu) << 8); v = (v >> 16) | (v << 16); return sw_uint_bits(v); }
inline float sw_ubfe(float a, float b, float c) { unsigned width = sw_bits_uint(a) & 31; unsigned offset = sw_bits_uint(b) & 31; if (width == 0) { return 0.f; } unsigned v = sw_bits_uint(c); return sw_uint_bits((v >> offset) & ((1u << width) - 1u)); }
inline float sw_ibfe(float a, float b, float c) { unsigned width = sw_bits_uint(a) & 31; unsigned offset = sw_bits_uint(b) & 31; if (width == 0) { return 0.f; } int v = sw_bits_int(c); v = v << (32 - width - offset); v = v >> (32 - width); return sw_int_bits(v); }
inline float sw_bfi(float a, float b, float c, float d) { unsigned width = sw_bits_uint(a) & 31; unsigned offset = sw_bits_uint(b) & 31; unsigned src = sw_bits_uint(c); unsigned dst = sw_bits_uint(d); if (width == 0) { return sw_uint_bits(dst); } unsigned mask = ((1u << width) - 1u) << offset; return sw_uint_bits((dst & ~mask) | ((src << offset) & mask)); }
inline float sw_rcp(float v) { return 1.f / v; }
inline float sw_f32tof16(float v)
{
    unsigned u = sw_bits_uint(v);
    unsigned sign = (u >> 16) & 0x8000u;
    int exp = ((u >> 23) & 0xFF) - 127;
    unsigned frac = u & 0x007FFFFFu;
    unsigned h;
    if (exp > 15)
    {
        h = sign | 0x7C00u;
    }
    else if (exp < -14)
    {
        h = sign;
    }
    else
    {
        h = sign | (unsigned)((exp + 15) << 10) | (frac >> 13);
    }
    return sw_uint_bits(h);
}
inline float sw_f16tof32(float v)
{
    unsigned h = sw_bits_uint(v) & 0xFFFFu;
    unsigned sign = (h & 0x8000u) << 16;
    unsigned exp  = (h >> 10) & 0x1F;
    unsigned frac = h & 0x03FFu;
    unsigned f;
    if (exp == 0)
    {
        f = sign;
    }
    else if (exp == 31)
    {
        f = sign | 0x7F800000u | (frac << 13);
    }
    else
    {
        f = sign | ((exp + 112) << 23) | (frac << 13);
    }
    return sw_uint_bits(f);
}

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

// Texture sampling
static inline const unsigned char* sw_mip_data(const SW_SRV& t, unsigned mip)
{
    mip = std::min(mip, t.mipLevels ? t.mipLevels - 1 : 0u);
    return static_cast<const unsigned char*>(t.data) + t.mipOffsets[mip];
}

static inline float sw_addr(float u, D3D11_TEXTURE_ADDRESS_MODE mode)
{
    switch (mode)
    {
    case D3D11_TEXTURE_ADDRESS_CLAMP:
        return std::fmin(std::fmax(u, 0.f), 1.f);
    case D3D11_TEXTURE_ADDRESS_MIRROR:
    {
        float t = std::fabs(u);
        float m = t - 2.f * std::floor(t * 0.5f);
        return m > 1.f ? 2.f - m : m;
    }
    case D3D11_TEXTURE_ADDRESS_MIRROR_ONCE:
        return std::fmin(std::fmax(std::fabs(u), 0.f), 1.f);
    case D3D11_TEXTURE_ADDRESS_BORDER:
        return u;
    default:
        return u - std::floor(u);
    }
}

static inline unsigned sw_format_stride(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:  return 16;
        case DXGI_FORMAT_R32G32B32_FLOAT:    return 12;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:        return 8;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SINT:        return 4;
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SINT:           return 2;
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SINT:            return 1;
        default:                             return 4;
    }
}

static inline float sw_half_to_float(unsigned short h)
{
    unsigned sign = (h & 0x8000u) << 16;
    unsigned exp  = (h >> 10) & 0x1F;
    unsigned frac = h & 0x03FFu;
    unsigned f;
    if (exp == 0)      { f = sign; }
    else if (exp == 31) { f = sign | 0x7F800000u | (frac << 13); }
    else               { f = sign | ((exp + 112) << 23) | (frac << 13); }
    float r;
    std::memcpy(&r, &f, 4);
    return r;
}

static inline float sw_r11_to_float(unsigned v)
{
    unsigned exp = (v >> 6) & 0x1F;
    unsigned frac = v & 0x3Fu;
    if (exp == 0) { return 0.f; }
    if (exp == 31) { return frac ? std::numeric_limits<float>::quiet_NaN() : std::numeric_limits<float>::infinity(); }
    unsigned f = ((exp + 112) << 23) | (frac << 17);
    float r;
    std::memcpy(&r, &f, 4);
    return r;
}

static inline float sw_r10_to_float(unsigned v)
{
    unsigned exp = (v >> 5) & 0x1F;
    unsigned frac = v & 0x1Fu;
    if (exp == 0) { return 0.f; }
    if (exp == 31) { return frac ? std::numeric_limits<float>::quiet_NaN() : std::numeric_limits<float>::infinity(); }
    unsigned f = ((exp + 112) << 23) | (frac << 18);
    float r;
    std::memcpy(&r, &f, 4);
    return r;
}

static inline SW_float4 sw_fetch_texel_at(const unsigned char* data, DXGI_FORMAT fmt,
                                           unsigned w, unsigned h, unsigned rowPitch,
                                           unsigned x, unsigned y)
{
    x = std::min(x, w ? w - 1 : 0u);
    y = std::min(y, h ? h - 1 : 0u);
    unsigned stride = sw_format_stride(fmt);
    const unsigned char* p = data + (unsigned long long)y * rowPitch + (unsigned long long)x * stride;
    switch (fmt)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return { p[0]/255.f, p[1]/255.f, p[2]/255.f, p[3]/255.f };
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return { p[2]/255.f, p[1]/255.f, p[0]/255.f, p[3]/255.f };
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return { std::fmax((signed char)p[0]/127.f,-1.f), std::fmax((signed char)p[1]/127.f,-1.f),
                     std::fmax((signed char)p[2]/127.f,-1.f), std::fmax((signed char)p[3]/127.f,-1.f) };
        case DXGI_FORMAT_R8G8B8A8_UINT:
            return { (float)p[0], (float)p[1], (float)p[2], (float)p[3] };
        case DXGI_FORMAT_R8G8B8A8_SINT:
            return { (float)(signed char)p[0], (float)(signed char)p[1],
                     (float)(signed char)p[2], (float)(signed char)p[3] };
        case DXGI_FORMAT_R8G8_UNORM:
            return { p[0]/255.f, p[1]/255.f, 0.f, 0.f };
        case DXGI_FORMAT_R8G8_SNORM:
            return { std::fmax((signed char)p[0]/127.f,-1.f), std::fmax((signed char)p[1]/127.f,-1.f), 0.f, 0.f };
        case DXGI_FORMAT_R8G8_UINT:
            return { (float)p[0], (float)p[1], 0.f, 0.f };
        case DXGI_FORMAT_R8G8_SINT:
            return { (float)(signed char)p[0], (float)(signed char)p[1], 0.f, 0.f };
        case DXGI_FORMAT_R8_UNORM:
            return { p[0]/255.f, 0.f, 0.f, 0.f };
        case DXGI_FORMAT_R8_SNORM:
            return { std::fmax((signed char)p[0]/127.f,-1.f), 0.f, 0.f, 0.f };
        case DXGI_FORMAT_R8_UINT:
            return { (float)p[0], 0.f, 0.f, 0.f };
        case DXGI_FORMAT_R8_SINT:
            return { (float)(signed char)p[0], 0.f, 0.f, 0.f };
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        {
            float v[4]; std::memcpy(v, p, 16);
            return { v[0], v[1], v[2], v[3] };
        }
        case DXGI_FORMAT_R32G32B32_FLOAT:
        {
            float v[3]; std::memcpy(v, p, 12);
            return { v[0], v[1], v[2], 0.f };
        }
        case DXGI_FORMAT_R32G32_FLOAT:
        {
            float v[2]; std::memcpy(v, p, 8);
            return { v[0], v[1], 0.f, 0.f };
        }
        case DXGI_FORMAT_R32_FLOAT:
        {
            float v; std::memcpy(&v, p, 4);
            return { v, 0.f, 0.f, 1.f };
        }
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        {
            unsigned v[4]; std::memcpy(v, p, 16);
            return { sw_uint_bits(v[0]), sw_uint_bits(v[1]), sw_uint_bits(v[2]), sw_uint_bits(v[3]) };
        }
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        {
            unsigned v[2]; std::memcpy(v, p, 8);
            return { sw_uint_bits(v[0]), sw_uint_bits(v[1]), 0.f, 0.f };
        }
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        {
            unsigned v; std::memcpy(&v, p, 4);
            return { sw_uint_bits(v), 0.f, 0.f, 0.f };
        }
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        {
            unsigned short v[4]; std::memcpy(v, p, 8);
            return { sw_half_to_float(v[0]), sw_half_to_float(v[1]), sw_half_to_float(v[2]), sw_half_to_float(v[3]) };
        }
        case DXGI_FORMAT_R16G16_FLOAT:
        {
            unsigned short v[2]; std::memcpy(v, p, 4);
            return { sw_half_to_float(v[0]), sw_half_to_float(v[1]), 0.f, 0.f };
        }
        case DXGI_FORMAT_R16_FLOAT:
        {
            unsigned short v; std::memcpy(&v, p, 2);
            return { sw_half_to_float(v), 0.f, 0.f, 0.f };
        }
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        {
            unsigned short v[4]; std::memcpy(v, p, 8);
            return { v[0]/65535.f, v[1]/65535.f, v[2]/65535.f, v[3]/65535.f };
        }
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        {
            short v[4]; std::memcpy(v, p, 8);
            return { std::fmax(v[0]/32767.f,-1.f), std::fmax(v[1]/32767.f,-1.f),
                     std::fmax(v[2]/32767.f,-1.f), std::fmax(v[3]/32767.f,-1.f) };
        }
        case DXGI_FORMAT_R16G16B16A16_UINT:
        {
            unsigned short v[4]; std::memcpy(v, p, 8);
            return { sw_uint_bits((unsigned)v[0]), sw_uint_bits((unsigned)v[1]),
                     sw_uint_bits((unsigned)v[2]), sw_uint_bits((unsigned)v[3]) };
        }
        case DXGI_FORMAT_R16G16B16A16_SINT:
        {
            short v[4]; std::memcpy(v, p, 8);
            return { sw_int_bits((int)v[0]), sw_int_bits((int)v[1]),
                     sw_int_bits((int)v[2]), sw_int_bits((int)v[3]) };
        }
        case DXGI_FORMAT_R16G16_UNORM:
        {
            unsigned short v[2]; std::memcpy(v, p, 4);
            return { v[0]/65535.f, v[1]/65535.f, 0.f, 0.f };
        }
        case DXGI_FORMAT_R16G16_SNORM:
        {
            short v[2]; std::memcpy(v, p, 4);
            return { std::fmax(v[0]/32767.f,-1.f), std::fmax(v[1]/32767.f,-1.f), 0.f, 0.f };
        }
        case DXGI_FORMAT_R16G16_UINT:
        {
            unsigned short v[2]; std::memcpy(v, p, 4);
            return { sw_uint_bits((unsigned)v[0]), sw_uint_bits((unsigned)v[1]), 0.f, 0.f };
        }
        case DXGI_FORMAT_R16G16_SINT:
        {
            short v[2]; std::memcpy(v, p, 4);
            return { sw_int_bits((int)v[0]), sw_int_bits((int)v[1]), 0.f, 0.f };
        }
        case DXGI_FORMAT_R16_UNORM:
        {
            unsigned short v; std::memcpy(&v, p, 2);
            return { v/65535.f, 0.f, 0.f, 0.f };
        }
        case DXGI_FORMAT_R16_SNORM:
        {
            short v; std::memcpy(&v, p, 2);
            return { std::fmax(v/32767.f,-1.f), 0.f, 0.f, 0.f };
        }
        case DXGI_FORMAT_R16_UINT:
        {
            unsigned short v; std::memcpy(&v, p, 2);
            return { sw_uint_bits((unsigned)v), 0.f, 0.f, 0.f };
        }
        case DXGI_FORMAT_R16_SINT:
        {
            short v; std::memcpy(&v, p, 2);
            return { sw_int_bits((int)v), 0.f, 0.f, 0.f };
        }
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        {
            unsigned packed; std::memcpy(&packed, p, 4);
            return { (packed & 0x3FFu)/1023.f, ((packed>>10)&0x3FFu)/1023.f,
                     ((packed>>20)&0x3FFu)/1023.f, ((packed>>30)&0x3u)/3.f };
        }
        case DXGI_FORMAT_R10G10B10A2_UINT:
        {
            unsigned packed; std::memcpy(&packed, p, 4);
            return { sw_uint_bits(packed & 0x3FFu), sw_uint_bits((packed>>10)&0x3FFu),
                     sw_uint_bits((packed>>20)&0x3FFu), sw_uint_bits((packed>>30)&0x3u) };
        }
        case DXGI_FORMAT_R11G11B10_FLOAT:
        {
            unsigned packed; std::memcpy(&packed, p, 4);
            return { sw_r11_to_float(packed & 0x7FFu), sw_r11_to_float((packed>>11)&0x7FFu),
                     sw_r10_to_float((packed>>22)&0x3FFu), 1.f };
        }
        default:
            return { 0.f, 0.f, 0.f, 0.f };
    }
}

static inline SW_float4 sw_fetch_texel(const SW_SRV& t, unsigned x, unsigned y)
{
    return sw_fetch_texel_at(static_cast<const unsigned char*>(t.data), t.format,
                             t.mips[0].width, t.mips[0].height, t.mips[0].rowPitch, x, y);
}

static inline SW_float4 sw_fetch_border_at(const unsigned char* data, DXGI_FORMAT fmt,
                                            unsigned w, unsigned h, unsigned rowPitch,
                                            int x, int y, bool borderU, bool borderV, const float bc[4])
{
    if ((borderU && (x < 0 || x >= (int)w)) ||
        (borderV && (y < 0 || y >= (int)h)))
    {
        return { bc[0], bc[1], bc[2], bc[3] };
    }
    return sw_fetch_texel_at(data, fmt, w, h, rowPitch,
                             (unsigned)std::clamp(x, 0, std::max((int)w - 1, 0)),
                             (unsigned)std::clamp(y, 0, std::max((int)h - 1, 0)));
}

static inline SW_float4 sw_fetch_border(const SW_SRV& t, int x, int y,
                                         bool borderU, bool borderV, const float bc[4])
{
    return sw_fetch_border_at(static_cast<const unsigned char*>(t.data), t.format,
                              t.mips[0].width, t.mips[0].height, t.mips[0].rowPitch,
                              x, y, borderU, borderV, bc);
}

static inline SW_float4 sw_fetch_texel_3d(const SW_SRV& t, unsigned x, unsigned y, unsigned z)
{
    z = std::min(z, t.mips[0].depth  ? t.mips[0].depth  - 1 : 0u);
    y = std::min(y, t.mips[0].height ? t.mips[0].height - 1 : 0u);
    x = std::min(x, t.mips[0].width  ? t.mips[0].width  - 1 : 0u);
    const unsigned char* base = static_cast<const unsigned char*>(t.data)
                               + (unsigned long long)z * t.mips[0].slicePitch
                               + (unsigned long long)y * t.mips[0].rowPitch;
    switch (t.format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        {
            const unsigned char* p = base + x * 4u;
            return { p[0]/255.f, p[1]/255.f, p[2]/255.f, p[3]/255.f };
        }
        case DXGI_FORMAT_R32_FLOAT:
        {
            float v;
            std::memcpy(&v, base + x * 4u, 4);
            return { v, v, v, v };
        }
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        {
            float v[4];
            std::memcpy(v, base + x * 16u, 16);
            return { v[0], v[1], v[2], v[3] };
        }
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        {
            const unsigned char* p = base + x * 4u;
            return { p[2]/255.f, p[1]/255.f, p[0]/255.f, p[3]/255.f };
        }
        default:
            return { 0.f, 0.f, 0.f, 0.f };
    }
}

static inline SW_float4 sw_resinfo_float(const SW_SRV& t)
{
    return { (float)t.mips[0].width, (float)t.mips[0].height, (float)t.mips[0].depth, (float)t.mipLevels };
}

static inline SW_float4 sw_resinfo_rcpfloat(const SW_SRV& t)
{
    return {
        t.mips[0].width  ? 1.f / (float)t.mips[0].width  : 0.f,
        t.mips[0].height ? 1.f / (float)t.mips[0].height : 0.f,
        t.mips[0].depth  ? 1.f / (float)t.mips[0].depth  : 0.f,
        (float)t.mipLevels
    };
}

static inline SW_float4 sw_resinfo_uint(const SW_SRV& t)
{
    return { sw_uint_bits(t.mips[0].width), sw_uint_bits(t.mips[0].height), sw_uint_bits(t.mips[0].depth), sw_uint_bits(t.mipLevels) };
}

static inline SW_float4 sw_bufinfo(const SW_UAV& u)
{
    return { sw_uint_bits(u.elementCount), 0.f, 0.f, 0.f };
}

static inline SW_float4 sw_sample_2d_at(const unsigned char* data, DXGI_FORMAT fmt,
                                         unsigned w, unsigned h, unsigned rowPitch,
                                         const SW_Sampler& s, float u, float v)
{
    float su = sw_addr(u, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressU));
    float sv = sw_addr(v, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressV));
    bool bU = (s.addressU == D3D11_TEXTURE_ADDRESS_BORDER);
    bool bV = (s.addressV == D3D11_TEXTURE_ADDRESS_BORDER);

    float fx = su * (float)w - 0.5f;
    float fy = sv * (float)h - 0.5f;

    if ((s.filter & 0x7F) == D3D11_FILTER_MIN_MAG_MIP_POINT ||
        (s.filter & 0x7F) == D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR)
    {
        int px = (int)(fx + 0.5f);
        int py = (int)(fy + 0.5f);
        return sw_fetch_border_at(data, fmt, w, h, rowPitch, px, py, bU, bV, s.borderColor);
    }

    int   x0 = (int)std::floor(fx);
    int   y0 = (int)std::floor(fy);
    float tx  = fx - (float)x0;
    float ty  = fy - (float)y0;

    SW_float4 s00 = sw_fetch_border_at(data, fmt, w, h, rowPitch, x0,   y0,   bU, bV, s.borderColor);
    SW_float4 s10 = sw_fetch_border_at(data, fmt, w, h, rowPitch, x0+1, y0,   bU, bV, s.borderColor);
    SW_float4 s01 = sw_fetch_border_at(data, fmt, w, h, rowPitch, x0,   y0+1, bU, bV, s.borderColor);
    SW_float4 s11 = sw_fetch_border_at(data, fmt, w, h, rowPitch, x0+1, y0+1, bU, bV, s.borderColor);

    auto lerp4 = [](SW_float4 a, SW_float4 b, float t) -> SW_float4 {
        return { a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t, a.w+(b.w-a.w)*t };
    };

    return lerp4(lerp4(s00, s10, tx), lerp4(s01, s11, tx), ty);
}

static inline SW_float4 sw_sample_2d(const SW_SRV& t, const SW_Sampler& s,
                                      float u, float v)
{
    return sw_sample_2d_at(static_cast<const unsigned char*>(t.data), t.format,
                           t.mips[0].width, t.mips[0].height, t.mips[0].rowPitch, s, u, v);
}

static inline SW_float4 sw_sample_1d(const SW_SRV& t, const SW_Sampler& s, float u)
{
    return sw_sample_2d(t, s, u, 0.5f);
}

static inline SW_float4 sw_sample_3d(const SW_SRV& t, const SW_Sampler& s,
                                      float u, float v, float w)
{
    float su = sw_addr(u, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressU));
    float sv = sw_addr(v, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressV));
    float sw_ = sw_addr(w, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressW));

    unsigned tw = t.mips[0].width;
    unsigned th = t.mips[0].height;
    unsigned td = t.mips[0].depth;

    float fz = sw_ * (float)td - 0.5f;
    int z0 = (int)std::floor(fz);
    int z1 = z0 + 1;
    float tz = fz - (float)z0;

    z0 = std::clamp(z0, 0, std::max((int)td - 1, 0));
    z1 = std::clamp(z1, 0, std::max((int)td - 1, 0));

    const unsigned char* base = static_cast<const unsigned char*>(t.data);
    unsigned rp = t.mips[0].rowPitch;
    unsigned sp = t.mips[0].slicePitch;

    if ((s.filter & 0x7F) == D3D11_FILTER_MIN_MAG_MIP_POINT ||
        (s.filter & 0x7F) == D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR)
    {
        int pz = (int)(fz + 0.5f);
        pz = std::clamp(pz, 0, std::max((int)td - 1, 0));
        return sw_sample_2d_at(base + (unsigned long long)pz * sp, t.format, tw, th, rp, s, u, v);
    }

    SW_float4 c0 = sw_sample_2d_at(base + (unsigned long long)z0 * sp, t.format, tw, th, rp, s, u, v);
    SW_float4 c1 = sw_sample_2d_at(base + (unsigned long long)z1 * sp, t.format, tw, th, rp, s, u, v);
    return {
        c0.x + (c1.x - c0.x) * tz,
        c0.y + (c1.y - c0.y) * tz,
        c0.z + (c1.z - c0.z) * tz,
        c0.w + (c1.w - c0.w) * tz
    };
}

static inline SW_float4 sw_sample_cube(const SW_SRV& t, const SW_Sampler& s,
                                        float x, float y, float z)
{
    float ax = std::fabs(x), ay = std::fabs(y), az = std::fabs(z);
    int face;
    float sc, tc, ma;
    if (ax >= ay && ax >= az)
    {
        face = x > 0.f ? 0 : 1;
        ma = ax;
        sc = x > 0.f ? -z : z;
        tc = -y;
    }
    else if (ay >= ax && ay >= az)
    {
        face = y > 0.f ? 2 : 3;
        ma = ay;
        sc = x;
        tc = y > 0.f ? z : -z;
    }
    else
    {
        face = z > 0.f ? 4 : 5;
        ma = az;
        sc = z > 0.f ? x : -x;
        tc = -y;
    }
    float fu = 0.5f * (sc / ma + 1.f);
    float fv = 0.5f * (tc / ma + 1.f);

    unsigned faceH = t.mips[0].height;
    unsigned rp = t.mips[0].rowPitch;
    unsigned sp = t.mips[0].slicePitch;
    const unsigned char* faceData = static_cast<const unsigned char*>(t.data)
                                    + (unsigned long long)face * sp;
    return sw_sample_2d_at(faceData, t.format, t.mips[0].width, faceH, rp, s, fu, fv);
}

static inline float sw_apply_cmp(D3D11_COMPARISON_FUNC fn, float val, float ref)
{
    switch (fn)
    {
    case D3D11_COMPARISON_NEVER:         return 0.f;
    case D3D11_COMPARISON_LESS:          return val < ref  ? 1.f : 0.f;
    case D3D11_COMPARISON_EQUAL:         return val == ref ? 1.f : 0.f;
    case D3D11_COMPARISON_LESS_EQUAL:    return val <= ref ? 1.f : 0.f;
    case D3D11_COMPARISON_GREATER:       return val > ref  ? 1.f : 0.f;
    case D3D11_COMPARISON_NOT_EQUAL:     return val != ref ? 1.f : 0.f;
    case D3D11_COMPARISON_GREATER_EQUAL: return val >= ref ? 1.f : 0.f;
    case D3D11_COMPARISON_ALWAYS:        return 1.f;
    default:                             return 0.f;
    }
}

static inline SW_float4 sw_sample_2d_cmp(const SW_SRV& t, const SW_Sampler& s,
                                          float u, float v, float ref)
{
    SW_float4 c = sw_sample_2d(t, s, u, v);
    float r = sw_apply_cmp(s.comparisonFunc, c.x, ref);
    return { r, r, r, r };
}

static inline SW_float4 sw_sample_2d_at_mip(const SW_SRV& t, const SW_Sampler& s,
                                              float u, float v, unsigned mip)
{
    mip = std::min(mip, t.mipLevels ? t.mipLevels - 1 : 0u);
    const unsigned char* data = sw_mip_data(t, mip);
    return sw_sample_2d_at(data, t.format,
                           t.mips[mip].width, t.mips[mip].height, t.mips[mip].rowPitch,
                           s, u, v);
}

static inline SW_float4 sw_sample_2d_lod(const SW_SRV& t, const SW_Sampler& s,
                                           float u, float v, float lod)
{
    lod += s.mipLODBias;
    lod = std::fmax(lod, s.minLOD);
    lod = std::fmin(lod, s.maxLOD);
    lod = std::fmax(lod, 0.f);
    float maxMip = t.mipLevels > 0 ? (float)(t.mipLevels - 1) : 0.f;
    lod = std::fmin(lod, maxMip);

    unsigned mipFilter = (s.filter >> 0) & 0x3;
    if (mipFilter == 0)
    {
        unsigned m = (unsigned)(lod + 0.5f);
        return sw_sample_2d_at_mip(t, s, u, v, m);
    }

    unsigned m0 = (unsigned)std::floor(lod);
    unsigned m1 = m0 + 1;
    float frac = lod - (float)m0;
    if (m1 >= t.mipLevels)
    {
        return sw_sample_2d_at_mip(t, s, u, v, m0);
    }

    SW_float4 c0 = sw_sample_2d_at_mip(t, s, u, v, m0);
    SW_float4 c1 = sw_sample_2d_at_mip(t, s, u, v, m1);
    return {
        c0.x + (c1.x - c0.x) * frac,
        c0.y + (c1.y - c0.y) * frac,
        c0.z + (c1.z - c0.z) * frac,
        c0.w + (c1.w - c0.w) * frac
    };
}

static inline float sw_compute_lod(const SW_SRV& t,
                                    float ddx_u, float ddx_v,
                                    float ddy_u, float ddy_v)
{
    float w = (float)t.mips[0].width;
    float h = (float)t.mips[0].height;
    float rho_x = std::sqrt(ddx_u * ddx_u * w * w + ddx_v * ddx_v * h * h);
    float rho_y = std::sqrt(ddy_u * ddy_u * w * w + ddy_v * ddy_v * h * h);
    float rho = std::fmax(rho_x, rho_y);
    return rho > 0.f ? std::log2(rho) : 0.f;
}

static inline SW_float4 sw_sample_2d_grad(const SW_SRV& t, const SW_Sampler& s,
                                            float u, float v,
                                            float ddx_u, float ddx_v,
                                            float ddy_u, float ddy_v)
{
    float lod = sw_compute_lod(t, ddx_u, ddx_v, ddy_u, ddy_v);
    return sw_sample_2d_lod(t, s, u, v, lod);
}

static inline SW_float4 sw_sample_2d_cmp_lod(const SW_SRV& t, const SW_Sampler& s,
                                               float u, float v, float ref, float lod)
{
    SW_float4 c = sw_sample_2d_lod(t, s, u, v, lod);
    float r = sw_apply_cmp(s.comparisonFunc, c.x, ref);
    return { r, r, r, r };
}

static inline SW_float4 sw_sample_2d_cmp_grad(const SW_SRV& t, const SW_Sampler& s,
                                                float u, float v, float ref,
                                                float ddx_u, float ddx_v,
                                                float ddy_u, float ddy_v)
{
    float lod = sw_compute_lod(t, ddx_u, ddx_v, ddy_u, ddy_v);
    SW_float4 c = sw_sample_2d_lod(t, s, u, v, lod);
    float r = sw_apply_cmp(s.comparisonFunc, c.x, ref);
    return { r, r, r, r };
}

static inline SW_float4 sw_sample_1d_lod(const SW_SRV& t, const SW_Sampler& s,
                                           float u, float lod)
{
    return sw_sample_2d_lod(t, s, u, 0.5f, lod);
}

static inline SW_float4 sw_sample_3d_lod(const SW_SRV& t, const SW_Sampler& s,
                                           float u, float v, float w, float lod)
{
    lod += s.mipLODBias;
    lod = std::fmax(lod, s.minLOD);
    lod = std::fmin(lod, s.maxLOD);
    lod = std::fmax(lod, 0.f);
    float maxMip = t.mipLevels > 0 ? (float)(t.mipLevels - 1) : 0.f;
    lod = std::fmin(lod, maxMip);

    unsigned mipFilter = (s.filter >> 0) & 0x3;
    bool pointZ = (s.filter & 0x7F) == D3D11_FILTER_MIN_MAG_MIP_POINT ||
                  (s.filter & 0x7F) == D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    auto sampleMip = [&](unsigned mip) -> SW_float4
    {
        mip = std::min(mip, t.mipLevels ? t.mipLevels - 1 : 0u);
        const unsigned char* data = sw_mip_data(t, mip);
        const SW_MipInfo& mi = t.mips[mip];
        float sw_ = sw_addr(w, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressW));
        float fz = sw_ * (float)mi.depth - 0.5f;
        if (pointZ)
        {
            int pz = std::clamp((int)(fz + 0.5f), 0, std::max((int)mi.depth - 1, 0));
            return sw_sample_2d_at(data + (unsigned long long)pz * mi.slicePitch,
                                    t.format, mi.width, mi.height, mi.rowPitch, s, u, v);
        }
        int z0 = std::clamp((int)std::floor(fz), 0, std::max((int)mi.depth - 1, 0));
        int z1 = std::clamp(z0 + 1, 0, std::max((int)mi.depth - 1, 0));
        float tz = fz - std::floor(fz);
        SW_float4 c0 = sw_sample_2d_at(data + (unsigned long long)z0 * mi.slicePitch,
                                         t.format, mi.width, mi.height, mi.rowPitch, s, u, v);
        SW_float4 c1 = sw_sample_2d_at(data + (unsigned long long)z1 * mi.slicePitch,
                                         t.format, mi.width, mi.height, mi.rowPitch, s, u, v);
        return { c0.x+(c1.x-c0.x)*tz, c0.y+(c1.y-c0.y)*tz, c0.z+(c1.z-c0.z)*tz, c0.w+(c1.w-c0.w)*tz };
    };

    if (mipFilter == 0)
    {
        return sampleMip((unsigned)(lod + 0.5f));
    }
    unsigned m0 = (unsigned)std::floor(lod);
    unsigned m1 = m0 + 1;
    float frac = lod - (float)m0;
    if (m1 >= t.mipLevels)
    {
        return sampleMip(m0);
    }
    SW_float4 c0 = sampleMip(m0);
    SW_float4 c1 = sampleMip(m1);
    return { c0.x+(c1.x-c0.x)*frac, c0.y+(c1.y-c0.y)*frac, c0.z+(c1.z-c0.z)*frac, c0.w+(c1.w-c0.w)*frac };
}

static inline SW_float4 sw_fetch_texel_mip(const SW_SRV& t, unsigned x, unsigned y, unsigned mip)
{
    mip = std::min(mip, t.mipLevels ? t.mipLevels - 1 : 0u);
    const unsigned char* data = sw_mip_data(t, mip);
    return sw_fetch_texel_at(data, t.format,
                             t.mips[mip].width, t.mips[mip].height, t.mips[mip].rowPitch, x, y);
}

static inline SW_float4 sw_resinfo_float_mip(const SW_SRV& t, unsigned mip)
{
    mip = std::min(mip, t.mipLevels ? t.mipLevels - 1 : 0u);
    return { (float)t.mips[mip].width, (float)t.mips[mip].height, (float)t.mips[mip].depth, (float)t.mipLevels };
}

static inline SW_float4 sw_resinfo_rcpfloat_mip(const SW_SRV& t, unsigned mip)
{
    mip = std::min(mip, t.mipLevels ? t.mipLevels - 1 : 0u);
    return {
        t.mips[mip].width  ? 1.f / (float)t.mips[mip].width  : 0.f,
        t.mips[mip].height ? 1.f / (float)t.mips[mip].height : 0.f,
        t.mips[mip].depth  ? 1.f / (float)t.mips[mip].depth  : 0.f,
        (float)t.mipLevels
    };
}

static inline SW_float4 sw_resinfo_uint_mip(const SW_SRV& t, unsigned mip)
{
    mip = std::min(mip, t.mipLevels ? t.mipLevels - 1 : 0u);
    return { sw_uint_bits(t.mips[mip].width), sw_uint_bits(t.mips[mip].height), sw_uint_bits(t.mips[mip].depth), sw_uint_bits(t.mipLevels) };
}

static inline SW_float4 sw_fetch_texel_3d_mip(const SW_SRV& t, unsigned x, unsigned y, unsigned z, unsigned mip)
{
    mip = std::min(mip, t.mipLevels ? t.mipLevels - 1 : 0u);
    const SW_MipInfo& mi = t.mips[mip];
    z = std::min(z, mi.depth  ? mi.depth  - 1 : 0u);
    y = std::min(y, mi.height ? mi.height - 1 : 0u);
    x = std::min(x, mi.width  ? mi.width  - 1 : 0u);
    const unsigned char* base = sw_mip_data(t, mip)
                               + (unsigned long long)z * mi.slicePitch
                               + (unsigned long long)y * mi.rowPitch;
    switch (t.format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        {
            const unsigned char* p = base + x * 4u;
            return { p[0]/255.f, p[1]/255.f, p[2]/255.f, p[3]/255.f };
        }
        case DXGI_FORMAT_R32_FLOAT:
        {
            float fv;
            std::memcpy(&fv, base + x * 4u, 4);
            return { fv, fv, fv, fv };
        }
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        {
            float fv[4];
            std::memcpy(fv, base + x * 16u, 16);
            return { fv[0], fv[1], fv[2], fv[3] };
        }
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        {
            const unsigned char* p = base + x * 4u;
            return { p[2]/255.f, p[1]/255.f, p[0]/255.f, p[3]/255.f };
        }
        default:
            return { 0.f, 0.f, 0.f, 0.f };
    }
}

static inline SW_float4 sw_gather_2d(const SW_SRV& t, const SW_Sampler& s,
                                      float u, float v, int comp)
{
    float su = sw_addr(u, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressU));
    float sv = sw_addr(v, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressV));
    bool bU = (s.addressU == D3D11_TEXTURE_ADDRESS_BORDER);
    bool bV = (s.addressV == D3D11_TEXTURE_ADDRESS_BORDER);

    float fx = su * (float)t.mips[0].width  - 0.5f;
    float fy = sv * (float)t.mips[0].height - 0.5f;

    int x0 = (int)std::floor(fx);
    int y0 = (int)std::floor(fy);

    SW_float4 s00 = sw_fetch_border(t, x0,   y0,   bU, bV, s.borderColor);
    SW_float4 s10 = sw_fetch_border(t, x0+1, y0,   bU, bV, s.borderColor);
    SW_float4 s01 = sw_fetch_border(t, x0,   y0+1, bU, bV, s.borderColor);
    SW_float4 s11 = sw_fetch_border(t, x0+1, y0+1, bU, bV, s.borderColor);

    const float* c00 = &s00.x;
    const float* c10 = &s10.x;
    const float* c01 = &s01.x;
    const float* c11 = &s11.x;

    return { c01[comp], c11[comp], c10[comp], c00[comp] };
}

static inline SW_float4 sw_gather_2d_cmp(const SW_SRV& t, const SW_Sampler& s,
                                          float u, float v, float ref, int comp)
{
    float su = sw_addr(u, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressU));
    float sv = sw_addr(v, static_cast<D3D11_TEXTURE_ADDRESS_MODE>(s.addressV));
    bool bU = (s.addressU == D3D11_TEXTURE_ADDRESS_BORDER);
    bool bV = (s.addressV == D3D11_TEXTURE_ADDRESS_BORDER);

    float fx = su * (float)t.mips[0].width  - 0.5f;
    float fy = sv * (float)t.mips[0].height - 0.5f;

    int x0 = (int)std::floor(fx);
    int y0 = (int)std::floor(fy);

    SW_float4 s00 = sw_fetch_border(t, x0,   y0,   bU, bV, s.borderColor);
    SW_float4 s10 = sw_fetch_border(t, x0+1, y0,   bU, bV, s.borderColor);
    SW_float4 s01 = sw_fetch_border(t, x0,   y0+1, bU, bV, s.borderColor);
    SW_float4 s11 = sw_fetch_border(t, x0+1, y0+1, bU, bV, s.borderColor);

    const float* c00 = &s00.x;
    const float* c10 = &s10.x;
    const float* c01 = &s01.x;
    const float* c11 = &s11.x;

    D3D11_COMPARISON_FUNC fn = s.comparisonFunc;
    return {
        sw_apply_cmp(fn, c01[comp], ref),
        sw_apply_cmp(fn, c11[comp], ref),
        sw_apply_cmp(fn, c10[comp], ref),
        sw_apply_cmp(fn, c00[comp], ref)
    };
}

static inline SW_float4 sw_uav_load_typed(const SW_UAV& u, unsigned x, unsigned y)
{
    unsigned idx = (u.dimension == D3D11_UAV_DIMENSION_BUFFER) ? x : y * u.width + x;
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

static inline void sw_uav_store_typed(SW_UAV& u, unsigned x, unsigned y, SW_float4 val)
{
    unsigned idx = (u.dimension == D3D11_UAV_DIMENSION_BUFFER) ? x : y * u.width + x;
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
            p[0] = (unsigned char)(sw_saturate(val.x) * 255.f);
            p[1] = (unsigned char)(sw_saturate(val.y) * 255.f);
            p[2] = (unsigned char)(sw_saturate(val.z) * 255.f);
            p[3] = (unsigned char)(sw_saturate(val.w) * 255.f);
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
    float f = sw_uint_bits(v);
    return { f, f, f, f };
}

static inline SW_float4 sw_srv_load_raw(const SW_SRV& t, unsigned byteOffset)
{
    if (!t.data) { return {0,0,0,0}; }
    unsigned v;
    std::memcpy(&v, static_cast<const unsigned char*>(t.data) + byteOffset, 4);
    float f = sw_uint_bits(v);
    return { f, f, f, f };
}

static inline void sw_uav_store_raw(SW_UAV& u, unsigned byteOffset, SW_float4 val)
{
    if (!u.data) { return; }
    unsigned v = sw_bits_uint(val.x);
    std::memcpy(static_cast<unsigned char*>(u.data) + byteOffset, &v, 4);
}

static inline SW_float4 sw_uav_load_structured(const SW_UAV& u, unsigned idx, unsigned byteOffset)
{
    if (!u.data || !u.stride) { return {0,0,0,0}; }
    unsigned base = idx * u.stride + byteOffset;
    unsigned v;
    std::memcpy(&v, static_cast<const unsigned char*>(u.data) + base, 4);
    float f = sw_uint_bits(v);
    return { f, f, f, f };
}

static inline SW_float4 sw_srv_load_structured(const SW_SRV& t, unsigned idx, unsigned byteOffset)
{
    if (!t.data || !t.stride) { return {0,0,0,0}; }
    unsigned base = idx * t.stride + byteOffset;
    unsigned v;
    std::memcpy(&v, static_cast<const unsigned char*>(t.data) + base, 4);
    float f = sw_uint_bits(v);
    return { f, f, f, f };
}

static inline void sw_uav_store_structured(SW_UAV& u, unsigned idx, unsigned byteOffset, SW_float4 val)
{
    if (!u.data || !u.stride) { return; }
    unsigned base = idx * u.stride + byteOffset;
    unsigned v = sw_bits_uint(val.x);
    std::memcpy(static_cast<unsigned char*>(u.data) + base, &v, 4);
}

// TGSM load/store
static inline SW_float4 sw_tgsm_load_raw(SW_TGSM& g, unsigned byteOffset)
{
    if (!g.data) { return {0,0,0,0}; }
    unsigned v;
    std::memcpy(&v, static_cast<const unsigned char*>(g.data) + byteOffset, 4);
    float f = sw_uint_bits(v);
    return { f, f, f, f };
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
    float f = sw_uint_bits(v);
    return { f, f, f, f };
}

static inline void sw_tgsm_store_structured(SW_TGSM& g, unsigned idx, unsigned byteOffset, SW_float4 val)
{
    if (!g.data || !g.stride) { return; }
    unsigned base = idx * g.stride + byteOffset;
    unsigned v = sw_bits_uint(val.x);
    std::memcpy(static_cast<unsigned char*>(g.data) + base, &v, 4);
}

static inline std::atomic_ref<unsigned> sw_uav_atomic_ref(SW_UAV& u, unsigned idx)
{
    return std::atomic_ref<unsigned>(*reinterpret_cast<unsigned*>(
        static_cast<unsigned char*>(u.data) + idx * 4u));
}

static inline std::atomic_ref<unsigned> sw_tgsm_atomic_ref(SW_TGSM& g, unsigned byteOffset)
{
    return std::atomic_ref<unsigned>(*reinterpret_cast<unsigned*>(
        static_cast<unsigned char*>(g.data) + byteOffset));
}

#define SW_DEF_ATOMIC_FETCH(name, method)                                                    \
static inline void sw_uav_##name(SW_UAV& u, unsigned idx, float val)                        \
{                                                                                            \
    sw_uav_atomic_ref(u, idx).method(sw_bits_uint(val), std::memory_order_relaxed);          \
}                                                                                            \
static inline float sw_uav_imm_##name(SW_UAV& u, unsigned idx, float val)                   \
{                                                                                            \
    unsigned old = sw_uav_atomic_ref(u, idx).method(sw_bits_uint(val),                       \
                                                    std::memory_order_relaxed);              \
    return sw_uint_bits(old);                                                                \
}                                                                                            \
static inline void sw_tgsm_##name(SW_TGSM& g, unsigned off, float val)                      \
{                                                                                            \
    sw_tgsm_atomic_ref(g, off).method(sw_bits_uint(val), std::memory_order_relaxed);         \
}                                                                                            \
static inline float sw_tgsm_imm_##name(SW_TGSM& g, unsigned off, float val)                 \
{                                                                                            \
    unsigned old = sw_tgsm_atomic_ref(g, off).method(sw_bits_uint(val),                      \
                                                     std::memory_order_relaxed);             \
    return sw_uint_bits(old);                                                                \
}

SW_DEF_ATOMIC_FETCH(atomic_iadd, fetch_add)
SW_DEF_ATOMIC_FETCH(atomic_and,  fetch_and)
SW_DEF_ATOMIC_FETCH(atomic_or,   fetch_or)
SW_DEF_ATOMIC_FETCH(atomic_xor,  fetch_xor)

#undef SW_DEF_ATOMIC_FETCH

#define SW_DEF_ATOMIC_CAS_LOOP(name, pick_new)                                               \
static inline void sw_uav_##name(SW_UAV& u, unsigned idx, float val)                        \
{                                                                                            \
    auto ref = sw_uav_atomic_ref(u, idx);                                                    \
    unsigned b = sw_bits_uint(val);                                                          \
    unsigned a = ref.load(std::memory_order_relaxed);                                        \
    while (!ref.compare_exchange_weak(a, (pick_new), std::memory_order_relaxed)) {}          \
}                                                                                            \
static inline float sw_uav_imm_##name(SW_UAV& u, unsigned idx, float val)                   \
{                                                                                            \
    auto ref = sw_uav_atomic_ref(u, idx);                                                    \
    unsigned b = sw_bits_uint(val);                                                          \
    unsigned a = ref.load(std::memory_order_relaxed);                                        \
    while (!ref.compare_exchange_weak(a, (pick_new), std::memory_order_relaxed)) {}          \
    return sw_uint_bits(a);                                                                  \
}                                                                                            \
static inline void sw_tgsm_##name(SW_TGSM& g, unsigned off, float val)                      \
{                                                                                            \
    auto ref = sw_tgsm_atomic_ref(g, off);                                                   \
    unsigned b = sw_bits_uint(val);                                                          \
    unsigned a = ref.load(std::memory_order_relaxed);                                        \
    while (!ref.compare_exchange_weak(a, (pick_new), std::memory_order_relaxed)) {}          \
}                                                                                            \
static inline float sw_tgsm_imm_##name(SW_TGSM& g, unsigned off, float val)                 \
{                                                                                            \
    auto ref = sw_tgsm_atomic_ref(g, off);                                                   \
    unsigned b = sw_bits_uint(val);                                                          \
    unsigned a = ref.load(std::memory_order_relaxed);                                        \
    while (!ref.compare_exchange_weak(a, (pick_new), std::memory_order_relaxed)) {}          \
    return sw_uint_bits(a);                                                                  \
}

SW_DEF_ATOMIC_CAS_LOOP(atomic_imax, ((int)a > (int)b) ? a : b)
SW_DEF_ATOMIC_CAS_LOOP(atomic_imin, ((int)a < (int)b) ? a : b)
SW_DEF_ATOMIC_CAS_LOOP(atomic_umax, (a > b) ? a : b)
SW_DEF_ATOMIC_CAS_LOOP(atomic_umin, (a < b) ? a : b)

#undef SW_DEF_ATOMIC_CAS_LOOP

static inline void sw_uav_atomic_cmp_store(SW_UAV& u, unsigned idx, float cmp, float val)
{
    unsigned expected = sw_bits_uint(cmp);
    sw_uav_atomic_ref(u, idx).compare_exchange_strong(expected, sw_bits_uint(val),
                                                      std::memory_order_relaxed);
}

static inline void sw_tgsm_atomic_cmp_store(SW_TGSM& g, unsigned off, float cmp, float val)
{
    unsigned expected = sw_bits_uint(cmp);
    sw_tgsm_atomic_ref(g, off).compare_exchange_strong(expected, sw_bits_uint(val),
                                                       std::memory_order_relaxed);
}

static inline float sw_uav_imm_atomic_exch(SW_UAV& u, unsigned idx, float val)
{
    return sw_uint_bits(sw_uav_atomic_ref(u, idx).exchange(sw_bits_uint(val),
                                                           std::memory_order_relaxed));
}

static inline float sw_tgsm_imm_atomic_exch(SW_TGSM& g, unsigned off, float val)
{
    return sw_uint_bits(sw_tgsm_atomic_ref(g, off).exchange(sw_bits_uint(val),
                                                            std::memory_order_relaxed));
}

static inline float sw_uav_imm_atomic_cmp_exch(SW_UAV& u, unsigned idx, float cmp, float val)
{
    unsigned expected = sw_bits_uint(cmp);
    sw_uav_atomic_ref(u, idx).compare_exchange_strong(expected, sw_bits_uint(val),
                                                      std::memory_order_relaxed);
    return sw_uint_bits(expected);
}

static inline float sw_tgsm_imm_atomic_cmp_exch(SW_TGSM& g, unsigned off, float cmp, float val)
{
    unsigned expected = sw_bits_uint(cmp);
    sw_tgsm_atomic_ref(g, off).compare_exchange_strong(expected, sw_bits_uint(val),
                                                       std::memory_order_relaxed);
    return sw_uint_bits(expected);
}
