#include <gtest/gtest.h>
#include "shaders/shader_runtime.h"
#include <cmath>

struct ShaderRuntimeTests : ::testing::Test {};

TEST_F(ShaderRuntimeTests, RcpPositive)
{
    EXPECT_FLOAT_EQ(sw_rcp(2.f), 0.5f);
    EXPECT_FLOAT_EQ(sw_rcp(4.f), 0.25f);
    EXPECT_FLOAT_EQ(sw_rcp(1.f), 1.f);
    EXPECT_FLOAT_EQ(sw_rcp(0.5f), 2.f);
}

TEST_F(ShaderRuntimeTests, RcpNegative)
{
    EXPECT_FLOAT_EQ(sw_rcp(-2.f), -0.5f);
    EXPECT_FLOAT_EQ(sw_rcp(-1.f), -1.f);
}

TEST_F(ShaderRuntimeTests, RcpZeroReturnsInf)
{
    EXPECT_TRUE(std::isinf(sw_rcp(0.f)));
}

TEST_F(ShaderRuntimeTests, F32toF16Roundtrip)
{
    float values[] = { 0.f, 1.f, -1.f, 0.5f, -0.5f, 1.5f, -3.25f, 65504.f };
    for (float v : values)
    {
        float packed = sw_f32tof16(v);
        float back   = sw_f16tof32(packed);
        EXPECT_NEAR(back, v, std::fabs(v) * 1e-3f + 1e-6f)
            << "Roundtrip failed for " << v;
    }
}

TEST_F(ShaderRuntimeTests, F32toF16Zero)
{
    float packed = sw_f32tof16(0.f);
    EXPECT_EQ(sw_bits_uint(packed), 0u);
    EXPECT_FLOAT_EQ(sw_f16tof32(packed), 0.f);
}

TEST_F(ShaderRuntimeTests, F32toF16One)
{
    float packed = sw_f32tof16(1.f);
    unsigned h = sw_bits_uint(packed);
    EXPECT_EQ(h, 0x3C00u);
}

TEST_F(ShaderRuntimeTests, F32toF16NegOne)
{
    float packed = sw_f32tof16(-1.f);
    unsigned h = sw_bits_uint(packed);
    EXPECT_EQ(h, 0xBC00u);
}

TEST_F(ShaderRuntimeTests, F32toF16Overflow)
{
    float packed = sw_f32tof16(100000.f);
    unsigned h = sw_bits_uint(packed);
    EXPECT_EQ(h & 0x7C00u, 0x7C00u);
}

TEST_F(ShaderRuntimeTests, F32toF16Denorm)
{
    float packed = sw_f32tof16(1e-8f);
    unsigned h = sw_bits_uint(packed);
    EXPECT_EQ(h, 0u);
}

TEST_F(ShaderRuntimeTests, F16toF32KnownValues)
{
    EXPECT_FLOAT_EQ(sw_f16tof32(sw_uint_bits(0x3C00u)), 1.f);
    EXPECT_FLOAT_EQ(sw_f16tof32(sw_uint_bits(0x4000u)), 2.f);
    EXPECT_FLOAT_EQ(sw_f16tof32(sw_uint_bits(0x3800u)), 0.5f);
    EXPECT_FLOAT_EQ(sw_f16tof32(sw_uint_bits(0xBC00u)), -1.f);
}

TEST_F(ShaderRuntimeTests, F16toF32Infinity)
{
    float result = sw_f16tof32(sw_uint_bits(0x7C00u));
    EXPECT_TRUE(std::isinf(result));
}

TEST_F(ShaderRuntimeTests, F16toF32Zero)
{
    EXPECT_FLOAT_EQ(sw_f16tof32(sw_uint_bits(0u)), 0.f);
}

TEST_F(ShaderRuntimeTests, UmulHi)
{
    EXPECT_EQ(sw_bits_uint(sw_umul_hi(sw_uint_bits(0x80000000u), sw_uint_bits(2u))), 1u);
    EXPECT_EQ(sw_bits_uint(sw_umul_hi(sw_uint_bits(3u), sw_uint_bits(5u))), 0u);
}

TEST_F(ShaderRuntimeTests, UmulLo)
{
    EXPECT_EQ(sw_bits_uint(sw_umul_lo(sw_uint_bits(3u), sw_uint_bits(5u))), 15u);
    EXPECT_EQ(sw_bits_uint(sw_umul_lo(sw_uint_bits(0xFFFFFFFFu), sw_uint_bits(2u))), 0xFFFFFFFEu);
}

TEST_F(ShaderRuntimeTests, Umad)
{
    EXPECT_EQ(sw_bits_uint(sw_umad(sw_uint_bits(3u), sw_uint_bits(5u), sw_uint_bits(10u))), 25u);
    EXPECT_EQ(sw_bits_uint(sw_umad(sw_uint_bits(0u), sw_uint_bits(5u), sw_uint_bits(7u))), 7u);
}

TEST_F(ShaderRuntimeTests, UaddcNoCarry)
{
    float carry;
    float r = sw_uaddc(sw_uint_bits(5u), sw_uint_bits(10u), carry);
    EXPECT_EQ(sw_bits_uint(r), 15u);
    EXPECT_EQ(sw_bits_uint(carry), 0u);
}

TEST_F(ShaderRuntimeTests, UaddcWithCarry)
{
    float carry;
    float r = sw_uaddc(sw_uint_bits(0xFFFFFFFFu), sw_uint_bits(1u), carry);
    EXPECT_EQ(sw_bits_uint(r), 0u);
    EXPECT_EQ(sw_bits_uint(carry), 1u);
}

TEST_F(ShaderRuntimeTests, UsubbNoBorrow)
{
    float borrow;
    float r = sw_usubb(sw_uint_bits(10u), sw_uint_bits(5u), borrow);
    EXPECT_EQ(sw_bits_uint(r), 5u);
    EXPECT_EQ(sw_bits_uint(borrow), 0u);
}

TEST_F(ShaderRuntimeTests, UsubbWithBorrow)
{
    float borrow;
    float r = sw_usubb(sw_uint_bits(0u), sw_uint_bits(1u), borrow);
    EXPECT_EQ(sw_bits_uint(r), 0xFFFFFFFFu);
    EXPECT_EQ(sw_bits_uint(borrow), 1u);
}
