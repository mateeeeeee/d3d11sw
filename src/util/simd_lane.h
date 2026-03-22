#pragma once
#include "common/types.h"
#include <algorithm>
#include <cmath>

#if defined(__ARM_NEON)
#define D3D11SW_SIMD_NEON 1
#include <arm_neon.h>
#elif defined(__SSE2__)
#define D3D11SW_SIMD_SSE 1
#include <immintrin.h>
#endif

namespace d3d11sw {

template<Usize N> struct LaneFloat;
template<Usize N> struct LaneMask;

template<> struct LaneMask<1>
{
    Bool v;

    static LaneMask Splat(Bool b) { return {b}; }

    LaneMask operator&(LaneMask r) const { return {v && r.v}; }
    LaneMask operator|(LaneMask r) const { return {v || r.v}; }
    LaneMask operator!() const { return {!v}; }

    Bool Any()  const { return v; }
    Bool All()  const { return v; }
    Bool None() const { return !v; }

    Int ActiveCount() const { return v ? 1 : 0; }
    Bool Active(Int) const { return v; }
    void SetInactive(Int) { v = false; }
};

template<> struct LaneFloat<1>
{
    Float v;

    static LaneFloat Splat(Float s) { return {s}; }
    static LaneFloat Iota(Float base, Float) { return {base}; }

    LaneFloat operator+(LaneFloat r) const { return {v + r.v}; }
    LaneFloat operator-(LaneFloat r) const { return {v - r.v}; }
    LaneFloat operator*(LaneFloat r) const { return {v * r.v}; }
    LaneFloat operator/(LaneFloat r) const { return {v / r.v}; }
    LaneFloat& operator+=(LaneFloat r) { v += r.v; return *this; }

    friend LaneFloat Min(LaneFloat a, LaneFloat b) { return {std::min(a.v, b.v)}; }
    friend LaneFloat Max(LaneFloat a, LaneFloat b) { return {std::max(a.v, b.v)}; }
    friend LaneFloat Clamp(LaneFloat x, LaneFloat lo, LaneFloat hi) { return {std::clamp(x.v, lo.v, hi.v)}; }
    friend LaneFloat Select(LaneMask<1> m, LaneFloat a, LaneFloat b) { return m.v ? a : b; }
    friend LaneFloat Abs(LaneFloat a) { return {std::abs(a.v)}; }

    LaneMask<1> operator>=(LaneFloat r) const { return {v >= r.v}; }
    LaneMask<1> operator<(LaneFloat r)  const { return {v < r.v}; }
    LaneMask<1> operator==(LaneFloat r) const { return {v == r.v}; }
    LaneMask<1> operator!=(LaneFloat r) const { return {v != r.v}; }

    Float Extract(Int) const { return v; }
    void Store(Float* p) const { *p = v; }
};

#if D3D11SW_SIMD_NEON

template<> struct LaneMask<4>
{
    uint32x4_t v;

    static LaneMask Splat(Bool b) { return {vdupq_n_u32(b ? 0xFFFFFFFFu : 0u)}; }

    LaneMask operator&(LaneMask r) const { return {vandq_u32(v, r.v)}; }
    LaneMask operator|(LaneMask r) const { return {vorrq_u32(v, r.v)}; }
    LaneMask operator!() const { return {vmvnq_u32(v)}; }

    Bool Any()  const { return vmaxvq_u32(v) != 0; }
    Bool All()  const { return vminvq_u32(v) != 0; }
    Bool None() const { return vmaxvq_u32(v) == 0; }

    Int ActiveCount() const
    {
        uint32_t bits[4];
        vst1q_u32(bits, v);
        return (bits[0] != 0) + (bits[1] != 0) + (bits[2] != 0) + (bits[3] != 0);
    }

    Bool Active(Int i) const
    {
        uint32_t bits[4];
        vst1q_u32(bits, v);
        return bits[i] != 0;
    }

    void SetInactive(Int i)
    {
        uint32_t bits[4];
        vst1q_u32(bits, v);
        bits[i] = 0;
        v = vld1q_u32(bits);
    }
};

template<> struct LaneFloat<4>
{
    float32x4_t v;

    static LaneFloat Splat(Float s) { return {vdupq_n_f32(s)}; }
    static LaneFloat Iota(Float base, Float step)
    {
        float vals[4] = {base, base + step, base + 2 * step, base + 3 * step};
        return {vld1q_f32(vals)};
    }

    LaneFloat operator+(LaneFloat r) const { return {vaddq_f32(v, r.v)}; }
    LaneFloat operator-(LaneFloat r) const { return {vsubq_f32(v, r.v)}; }
    LaneFloat operator*(LaneFloat r) const { return {vmulq_f32(v, r.v)}; }
    LaneFloat operator/(LaneFloat r) const { return {vdivq_f32(v, r.v)}; }
    LaneFloat& operator+=(LaneFloat r) { v = vaddq_f32(v, r.v); return *this; }

    friend LaneFloat Min(LaneFloat a, LaneFloat b) { return {vminq_f32(a.v, b.v)}; }
    friend LaneFloat Max(LaneFloat a, LaneFloat b) { return {vmaxq_f32(a.v, b.v)}; }
    friend LaneFloat Clamp(LaneFloat x, LaneFloat lo, LaneFloat hi) { return Min(Max(x, lo), hi); }
    friend LaneFloat Select(LaneMask<4> m, LaneFloat a, LaneFloat b) { return {vbslq_f32(m.v, a.v, b.v)}; }
    friend LaneFloat Abs(LaneFloat a) { return {vabsq_f32(a.v)}; }

    LaneMask<4> operator>=(LaneFloat r) const { return {vcgeq_f32(v, r.v)}; }
    LaneMask<4> operator<(LaneFloat r)  const { return {vcltq_f32(v, r.v)}; }
    LaneMask<4> operator==(LaneFloat r) const { return {vceqq_f32(v, r.v)}; }
    LaneMask<4> operator!=(LaneFloat r) const { return {vmvnq_u32(vceqq_f32(v, r.v))}; }

    Float Extract(Int i) const
    {
        float vals[4];
        vst1q_f32(vals, v);
        return vals[i];
    }

    void Store(Float* p) const { vst1q_f32(p, v); }
};

#elif D3D11SW_SIMD_SSE

template<> struct LaneMask<4>
{
    __m128 v;

    static LaneMask Splat(Bool b)
    {
        return {_mm_castsi128_ps(_mm_set1_epi32(b ? -1 : 0))};
    }

    LaneMask operator&(LaneMask r) const { return {_mm_and_ps(v, r.v)}; }
    LaneMask operator|(LaneMask r) const { return {_mm_or_ps(v, r.v)}; }
    LaneMask operator!() const { return {_mm_xor_ps(v, _mm_castsi128_ps(_mm_set1_epi32(-1)))}; }

    Bool Any()  const { return _mm_movemask_ps(v) != 0; }
    Bool All()  const { return _mm_movemask_ps(v) == 0xF; }
    Bool None() const { return _mm_movemask_ps(v) == 0; }

    Int ActiveCount() const
    {
        Int bits = _mm_movemask_ps(v);
        return (bits & 1) + ((bits >> 1) & 1) + ((bits >> 2) & 1) + ((bits >> 3) & 1);
    }

    Bool Active(Int i) const { return (_mm_movemask_ps(v) >> i) & 1; }

    void SetInactive(Int i)
    {
        alignas(16) Uint32 bits[4];
        _mm_store_ps(reinterpret_cast<float*>(bits), v);
        bits[i] = 0;
        v = _mm_load_ps(reinterpret_cast<float*>(bits));
    }
};

template<> struct LaneFloat<4>
{
    __m128 v;

    static LaneFloat Splat(Float s) { return {_mm_set1_ps(s)}; }
    static LaneFloat Iota(Float base, Float step)
    {
        return {_mm_set_ps(base + 3 * step, base + 2 * step, base + step, base)};
    }

    LaneFloat operator+(LaneFloat r) const { return {_mm_add_ps(v, r.v)}; }
    LaneFloat operator-(LaneFloat r) const { return {_mm_sub_ps(v, r.v)}; }
    LaneFloat operator*(LaneFloat r) const { return {_mm_mul_ps(v, r.v)}; }
    LaneFloat operator/(LaneFloat r) const { return {_mm_div_ps(v, r.v)}; }
    LaneFloat& operator+=(LaneFloat r) { v = _mm_add_ps(v, r.v); return *this; }

    friend LaneFloat Min(LaneFloat a, LaneFloat b) { return {_mm_min_ps(a.v, b.v)}; }
    friend LaneFloat Max(LaneFloat a, LaneFloat b) { return {_mm_max_ps(a.v, b.v)}; }
    friend LaneFloat Clamp(LaneFloat x, LaneFloat lo, LaneFloat hi) { return Min(Max(x, lo), hi); }
    friend LaneFloat Select(LaneMask<4> m, LaneFloat a, LaneFloat b)
    {
        return {_mm_or_ps(_mm_and_ps(m.v, a.v), _mm_andnot_ps(m.v, b.v))};
    }
    friend LaneFloat Abs(LaneFloat a)
    {
        return {_mm_and_ps(a.v, _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)))};
    }

    LaneMask<4> operator>=(LaneFloat r) const { return {_mm_cmpge_ps(v, r.v)}; }
    LaneMask<4> operator<(LaneFloat r)  const { return {_mm_cmplt_ps(v, r.v)}; }
    LaneMask<4> operator==(LaneFloat r) const { return {_mm_cmpeq_ps(v, r.v)}; }
    LaneMask<4> operator!=(LaneFloat r) const { return {_mm_cmpneq_ps(v, r.v)}; }

    Float Extract(Int i) const
    {
        alignas(16) float vals[4];
        _mm_store_ps(vals, v);
        return vals[i];
    }

    void Store(Float* p) const { _mm_storeu_ps(p, v); }
};

#else

template<> struct LaneMask<4>
{
    Uint32 v[4];

    static LaneMask Splat(Bool b)
    {
        Uint32 m = b ? 0xFFFFFFFFu : 0u;
        return {{m, m, m, m}};
    }

    LaneMask operator&(LaneMask r) const { return {{v[0]&r.v[0], v[1]&r.v[1], v[2]&r.v[2], v[3]&r.v[3]}}; }
    LaneMask operator|(LaneMask r) const { return {{v[0]|r.v[0], v[1]|r.v[1], v[2]|r.v[2], v[3]|r.v[3]}}; }
    LaneMask operator!() const { return {{~v[0], ~v[1], ~v[2], ~v[3]}}; }

    Bool Any()  const { return (v[0] | v[1] | v[2] | v[3]) != 0; }
    Bool All()  const { return (v[0] & v[1] & v[2] & v[3]) != 0; }
    Bool None() const { return !Any(); }

    Int ActiveCount() const { return (v[0]!=0) + (v[1]!=0) + (v[2]!=0) + (v[3]!=0); }
    Bool Active(Int i) const { return v[i] != 0; }
    void SetInactive(Int i) { v[i] = 0; }
};

template<> struct LaneFloat<4>
{
    Float v[4];

    static LaneFloat Splat(Float s) { return {{s, s, s, s}}; }
    static LaneFloat Iota(Float base, Float step)
    {
        return {{base, base + step, base + 2 * step, base + 3 * step}};
    }

    LaneFloat operator+(LaneFloat r) const { return {{v[0]+r.v[0], v[1]+r.v[1], v[2]+r.v[2], v[3]+r.v[3]}}; }
    LaneFloat operator-(LaneFloat r) const { return {{v[0]-r.v[0], v[1]-r.v[1], v[2]-r.v[2], v[3]-r.v[3]}}; }
    LaneFloat operator*(LaneFloat r) const { return {{v[0]*r.v[0], v[1]*r.v[1], v[2]*r.v[2], v[3]*r.v[3]}}; }
    LaneFloat operator/(LaneFloat r) const { return {{v[0]/r.v[0], v[1]/r.v[1], v[2]/r.v[2], v[3]/r.v[3]}}; }
    LaneFloat& operator+=(LaneFloat r) { for (Int i = 0; i < 4; ++i) { v[i] += r.v[i]; } return *this; }

    friend LaneFloat Min(LaneFloat a, LaneFloat b)
    {
        return {{std::min(a.v[0],b.v[0]), std::min(a.v[1],b.v[1]), std::min(a.v[2],b.v[2]), std::min(a.v[3],b.v[3])}};
    }
    friend LaneFloat Max(LaneFloat a, LaneFloat b)
    {
        return {{std::max(a.v[0],b.v[0]), std::max(a.v[1],b.v[1]), std::max(a.v[2],b.v[2]), std::max(a.v[3],b.v[3])}};
    }
    friend LaneFloat Clamp(LaneFloat x, LaneFloat lo, LaneFloat hi) { return Min(Max(x, lo), hi); }
    friend LaneFloat Select(LaneMask<4> m, LaneFloat a, LaneFloat b)
    {
        return {{m.v[0]?a.v[0]:b.v[0], m.v[1]?a.v[1]:b.v[1], m.v[2]?a.v[2]:b.v[2], m.v[3]?a.v[3]:b.v[3]}};
    }
    friend LaneFloat Abs(LaneFloat a)
    {
        return {{std::abs(a.v[0]), std::abs(a.v[1]), std::abs(a.v[2]), std::abs(a.v[3])}};
    }

    LaneMask<4> operator>=(LaneFloat r) const
    {
        return {{v[0]>=r.v[0]?~0u:0u, v[1]>=r.v[1]?~0u:0u, v[2]>=r.v[2]?~0u:0u, v[3]>=r.v[3]?~0u:0u}};
    }
    LaneMask<4> operator<(LaneFloat r) const
    {
        return {{v[0]<r.v[0]?~0u:0u, v[1]<r.v[1]?~0u:0u, v[2]<r.v[2]?~0u:0u, v[3]<r.v[3]?~0u:0u}};
    }
    LaneMask<4> operator==(LaneFloat r) const
    {
        return {{v[0]==r.v[0]?~0u:0u, v[1]==r.v[1]?~0u:0u, v[2]==r.v[2]?~0u:0u, v[3]==r.v[3]?~0u:0u}};
    }
    LaneMask<4> operator!=(LaneFloat r) const
    {
        return {{v[0]!=r.v[0]?~0u:0u, v[1]!=r.v[1]?~0u:0u, v[2]!=r.v[2]?~0u:0u, v[3]!=r.v[3]?~0u:0u}};
    }

    Float Extract(Int i) const { return v[i]; }
    void Store(Float* p) const { for (Int i = 0; i < 4; ++i) { p[i] = v[i]; } }
};

#endif

#if D3D11SW_SIMD_NEON || D3D11SW_SIMD_SSE
inline constexpr Usize SimdWidth = 4;
#else
inline constexpr Usize SimdWidth = 1;
#endif

using LaneF = LaneFloat<SimdWidth>;
using LaneM = LaneMask<SimdWidth>;

struct SimdVec4
{
#if D3D11SW_SIMD_NEON
    float32x4_t v;

    static SimdVec4 Load(const Float* p) { return {vld1q_f32(p)}; }
    void Store(Float* p) const { vst1q_f32(p, v); }

    SimdVec4 operator+(SimdVec4 r) const { return {vaddq_f32(v, r.v)}; }
    SimdVec4 operator-(SimdVec4 r) const { return {vsubq_f32(v, r.v)}; }
    SimdVec4 operator*(SimdVec4 r) const { return {vmulq_f32(v, r.v)}; }

    friend SimdVec4 MulScalar(SimdVec4 a, Float s) { return {vmulq_n_f32(a.v, s)}; }
    friend SimdVec4 MulAdd(SimdVec4 a, SimdVec4 b, Float s) { return {vmlaq_n_f32(a.v, b.v, s)}; }

#elif D3D11SW_SIMD_SSE
    __m128 v;

    static SimdVec4 Load(const Float* p) { return {_mm_loadu_ps(p)}; }
    void Store(Float* p) const { _mm_storeu_ps(p, v); }

    SimdVec4 operator+(SimdVec4 r) const { return {_mm_add_ps(v, r.v)}; }
    SimdVec4 operator-(SimdVec4 r) const { return {_mm_sub_ps(v, r.v)}; }
    SimdVec4 operator*(SimdVec4 r) const { return {_mm_mul_ps(v, r.v)}; }

    friend SimdVec4 MulScalar(SimdVec4 a, Float s) { return {_mm_mul_ps(a.v, _mm_set1_ps(s))}; }
    friend SimdVec4 MulAdd(SimdVec4 a, SimdVec4 b, Float s)
    {
        return {_mm_add_ps(a.v, _mm_mul_ps(b.v, _mm_set1_ps(s)))};
    }

#else
    Float v[4];

    static SimdVec4 Load(const Float* p) { return {{p[0], p[1], p[2], p[3]}}; }
    void Store(Float* p) const { p[0] = v[0]; p[1] = v[1]; p[2] = v[2]; p[3] = v[3]; }

    SimdVec4 operator+(SimdVec4 r) const { return {{v[0]+r.v[0], v[1]+r.v[1], v[2]+r.v[2], v[3]+r.v[3]}}; }
    SimdVec4 operator-(SimdVec4 r) const { return {{v[0]-r.v[0], v[1]-r.v[1], v[2]-r.v[2], v[3]-r.v[3]}}; }
    SimdVec4 operator*(SimdVec4 r) const { return {{v[0]*r.v[0], v[1]*r.v[1], v[2]*r.v[2], v[3]*r.v[3]}}; }

    friend SimdVec4 MulScalar(SimdVec4 a, Float s) { return {{a.v[0]*s, a.v[1]*s, a.v[2]*s, a.v[3]*s}}; }
    friend SimdVec4 MulAdd(SimdVec4 a, SimdVec4 b, Float s)
    {
        return {{a.v[0]+b.v[0]*s, a.v[1]+b.v[1]*s, a.v[2]+b.v[2]*s, a.v[3]+b.v[3]*s}};
    }
#endif
};

}
