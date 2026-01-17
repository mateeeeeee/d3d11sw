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

TEST_F(ShaderRuntimeTests, FetchTexel1D)
{
    float data[] = { 1.f, 2.f, 3.f, 4.f };
    SW_SRV t{};
    t.data     = data;
    t.format   = DXGI_FORMAT_R32_FLOAT;
    t.mips[0].width    = 4;
    t.mips[0].height   = 1;
    t.mips[0].depth    = 1;
    t.mips[0].rowPitch = 4 * 4;
    t.mips[0].slicePitch = t.mips[0].rowPitch;
    t.mipLevels = 1;

    SW_float4 r0 = sw_fetch_texel(t, 0, 0);
    SW_float4 r2 = sw_fetch_texel(t, 2, 0);
    EXPECT_FLOAT_EQ(r0.x, 1.f);
    EXPECT_FLOAT_EQ(r2.x, 3.f);
}

TEST_F(ShaderRuntimeTests, FetchTexel3d)
{
    unsigned char data[2][2][2][4];
    for (int z = 0; z < 2; ++z)
    {
        for (int y = 0; y < 2; ++y)
        {
            for (int x = 0; x < 2; ++x)
            {
                data[z][y][x][0] = (unsigned char)(z * 100 + y * 10 + x);
                data[z][y][x][1] = 0;
                data[z][y][x][2] = 0;
                data[z][y][x][3] = 255;
            }
        }
    }

    SW_SRV t{};
    t.data       = data;
    t.format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    t.mips[0].width      = 2;
    t.mips[0].height     = 2;
    t.mips[0].depth      = 2;
    t.mips[0].rowPitch   = 2 * 4;
    t.mips[0].slicePitch = 2 * 2 * 4;
    t.mipLevels = 1;

    SW_float4 r000 = sw_fetch_texel_3d(t, 0, 0, 0);
    SW_float4 r100 = sw_fetch_texel_3d(t, 1, 0, 0);
    SW_float4 r010 = sw_fetch_texel_3d(t, 0, 1, 0);
    SW_float4 r001 = sw_fetch_texel_3d(t, 0, 0, 1);
    SW_float4 r111 = sw_fetch_texel_3d(t, 1, 1, 1);

    EXPECT_NEAR(r000.x, 0.f / 255.f, 1e-3f);
    EXPECT_NEAR(r100.x, 1.f / 255.f, 1e-3f);
    EXPECT_NEAR(r010.x, 10.f / 255.f, 1e-3f);
    EXPECT_NEAR(r001.x, 100.f / 255.f, 1e-3f);
    EXPECT_NEAR(r111.x, 111.f / 255.f, 1e-3f);
}

TEST_F(ShaderRuntimeTests, FetchTexel3dClamps)
{
    float data[4] = { 1.f, 2.f, 3.f, 4.f };
    SW_SRV t{};
    t.data       = data;
    t.format     = DXGI_FORMAT_R32_FLOAT;
    t.mips[0].width      = 2;
    t.mips[0].height     = 1;
    t.mips[0].depth      = 1;
    t.mips[0].rowPitch   = 2 * 4;
    t.mips[0].slicePitch = 2 * 4;
    t.mipLevels = 1;

    SW_float4 r = sw_fetch_texel_3d(t, 99, 99, 99);
    EXPECT_FLOAT_EQ(r.x, 2.f);
}

TEST_F(ShaderRuntimeTests, FetchTexel2dVia3d)
{
    float data[] = { 10.f, 20.f, 30.f, 40.f };
    SW_SRV t{};
    t.data       = data;
    t.format     = DXGI_FORMAT_R32_FLOAT;
    t.mips[0].width      = 2;
    t.mips[0].height     = 2;
    t.mips[0].depth      = 1;
    t.mips[0].rowPitch   = 2 * 4;
    t.mips[0].slicePitch = 2 * 2 * 4;
    t.mipLevels = 1;

    SW_float4 r00 = sw_fetch_texel_3d(t, 0, 0, 0);
    SW_float4 r10 = sw_fetch_texel_3d(t, 1, 0, 0);
    SW_float4 r01 = sw_fetch_texel_3d(t, 0, 1, 0);
    SW_float4 r11 = sw_fetch_texel_3d(t, 1, 1, 0);
    EXPECT_FLOAT_EQ(r00.x, 10.f);
    EXPECT_FLOAT_EQ(r10.x, 20.f);
    EXPECT_FLOAT_EQ(r01.x, 30.f);
    EXPECT_FLOAT_EQ(r11.x, 40.f);
}

TEST_F(ShaderRuntimeTests, ResinfoFloat)
{
    SW_SRV t{};
    t.mips[0].width    = 256;
    t.mips[0].height   = 128;
    t.mips[0].depth    = 1;
    t.mipLevels = 9;

    SW_float4 r = sw_resinfo_float(t);
    EXPECT_FLOAT_EQ(r.x, 256.f);
    EXPECT_FLOAT_EQ(r.y, 128.f);
    EXPECT_FLOAT_EQ(r.z, 1.f);
    EXPECT_FLOAT_EQ(r.w, 9.f);
}

TEST_F(ShaderRuntimeTests, ResinfoRcpFloat)
{
    SW_SRV t{};
    t.mips[0].width    = 4;
    t.mips[0].height   = 2;
    t.mips[0].depth    = 1;
    t.mipLevels = 3;

    SW_float4 r = sw_resinfo_rcpfloat(t);
    EXPECT_FLOAT_EQ(r.x, 0.25f);
    EXPECT_FLOAT_EQ(r.y, 0.5f);
    EXPECT_FLOAT_EQ(r.z, 1.f);
    EXPECT_FLOAT_EQ(r.w, 3.f);
}

TEST_F(ShaderRuntimeTests, ResinfoUint)
{
    SW_SRV t{};
    t.mips[0].width    = 512;
    t.mips[0].height   = 256;
    t.mips[0].depth    = 64;
    t.mipLevels = 10;

    SW_float4 r = sw_resinfo_uint(t);
    EXPECT_EQ(sw_bits_uint(r.x), 512u);
    EXPECT_EQ(sw_bits_uint(r.y), 256u);
    EXPECT_EQ(sw_bits_uint(r.z), 64u);
    EXPECT_EQ(sw_bits_uint(r.w), 10u);
}

TEST_F(ShaderRuntimeTests, FetchTexelMip)
{
    // 2x2 mip0 (red), 1x1 mip1 (green)
    unsigned char mip0[2][2][4] = {
        {{255,0,0,255}, {255,0,0,255}},
        {{255,0,0,255}, {255,0,0,255}},
    };
    unsigned char mip1[1][1][4] = {
        {{0,255,0,255}},
    };

    unsigned char data[sizeof(mip0) + sizeof(mip1)];
    std::memcpy(data, mip0, sizeof(mip0));
    std::memcpy(data + sizeof(mip0), mip1, sizeof(mip1));

    SW_SRV t{};
    t.data = data;
    t.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    t.mipLevels = 2;
    t.mips[0] = {2, 2, 1, 2*4, 2*2*4};
    t.mips[1] = {1, 1, 1, 1*4, 1*4};
    t.mipOffsets[0] = 0;
    t.mipOffsets[1] = sizeof(mip0);

    SW_float4 r0 = sw_fetch_texel_mip(t, 0, 0, 0);
    EXPECT_NEAR(r0.x, 1.f, 1e-3f);
    EXPECT_NEAR(r0.y, 0.f, 1e-3f);

    SW_float4 r1 = sw_fetch_texel_mip(t, 0, 0, 1);
    EXPECT_NEAR(r1.x, 0.f, 1e-3f);
    EXPECT_NEAR(r1.y, 1.f, 1e-3f);
}

TEST_F(ShaderRuntimeTests, FetchTexelMipClamps)
{
    unsigned char mip0[4] = {255, 0, 0, 255};
    SW_SRV t{};
    t.data = mip0;
    t.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    t.mipLevels = 1;
    t.mips[0] = {1, 1, 1, 4, 4};
    t.mipOffsets[0] = 0;

    SW_float4 r = sw_fetch_texel_mip(t, 0, 0, 99);
    EXPECT_NEAR(r.x, 1.f, 1e-3f);
}

TEST_F(ShaderRuntimeTests, MipData)
{
    unsigned char data[32] = {};
    data[0] = 0xAA;
    data[16] = 0xBB;

    SW_SRV t{};
    t.data = data;
    t.mipLevels = 2;
    t.mipOffsets[0] = 0;
    t.mipOffsets[1] = 16;

    EXPECT_EQ(sw_mip_data(t, 0)[0], 0xAA);
    EXPECT_EQ(sw_mip_data(t, 1)[0], 0xBB);
    EXPECT_EQ(sw_mip_data(t, 99)[0], 0xBB);
}

TEST_F(ShaderRuntimeTests, SampleLodPoint)
{
    unsigned char mip0[2][2][4] = {
        {{255,0,0,255}, {255,0,0,255}},
        {{255,0,0,255}, {255,0,0,255}},
    };
    unsigned char mip1[1][1][4] = {
        {{0,255,0,255}},
    };

    unsigned char data[sizeof(mip0) + sizeof(mip1)];
    std::memcpy(data, mip0, sizeof(mip0));
    std::memcpy(data + sizeof(mip0), mip1, sizeof(mip1));

    SW_SRV t{};
    t.data = data;
    t.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    t.mipLevels = 2;
    t.mips[0] = {2, 2, 1, 2*4, 2*2*4};
    t.mips[1] = {1, 1, 1, 1*4, 1*4};
    t.mipOffsets[0] = 0;
    t.mipOffsets[1] = sizeof(mip0);

    SW_Sampler s{};
    s.filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    s.addressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    s.addressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    s.maxLOD = 1.f;

    SW_float4 r0 = sw_sample_2d_lod(t, s, 0.5f, 0.5f, 0.f);
    EXPECT_NEAR(r0.x, 1.f, 1e-3f);
    EXPECT_NEAR(r0.y, 0.f, 1e-3f);

    SW_float4 r1 = sw_sample_2d_lod(t, s, 0.5f, 0.5f, 1.f);
    EXPECT_NEAR(r1.x, 0.f, 1e-3f);
    EXPECT_NEAR(r1.y, 1.f, 1e-3f);
}

TEST_F(ShaderRuntimeTests, SampleLodTrilinear)
{
    unsigned char mip0[1][1][4] = {{{255,0,0,255}}};
    unsigned char mip1[1][1][4] = {{{0,255,0,255}}};

    unsigned char data[sizeof(mip0) + sizeof(mip1)];
    std::memcpy(data, mip0, sizeof(mip0));
    std::memcpy(data + sizeof(mip0), mip1, sizeof(mip1));

    SW_SRV t{};
    t.data = data;
    t.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    t.mipLevels = 2;
    t.mips[0] = {1, 1, 1, 4, 4};
    t.mips[1] = {1, 1, 1, 4, 4};
    t.mipOffsets[0] = 0;
    t.mipOffsets[1] = sizeof(mip0);

    SW_Sampler s{};
    s.filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    s.addressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    s.addressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    s.maxLOD = 1.f;

    SW_float4 r = sw_sample_2d_lod(t, s, 0.5f, 0.5f, 0.5f);
    EXPECT_NEAR(r.x, 0.5f, 0.02f);
    EXPECT_NEAR(r.y, 0.5f, 0.02f);
}

TEST_F(ShaderRuntimeTests, SampleLodClampsToMinMax)
{
    unsigned char mip0[1][1][4] = {{{255,0,0,255}}};
    unsigned char mip1[1][1][4] = {{{0,255,0,255}}};

    unsigned char data[sizeof(mip0) + sizeof(mip1)];
    std::memcpy(data, mip0, sizeof(mip0));
    std::memcpy(data + sizeof(mip0), mip1, sizeof(mip1));

    SW_SRV t{};
    t.data = data;
    t.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    t.mipLevels = 2;
    t.mips[0] = {1, 1, 1, 4, 4};
    t.mips[1] = {1, 1, 1, 4, 4};
    t.mipOffsets[0] = 0;
    t.mipOffsets[1] = sizeof(mip0);

    SW_Sampler s{};
    s.filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    s.addressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    s.addressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    s.minLOD = 0.f;
    s.maxLOD = 0.f;

    SW_float4 r = sw_sample_2d_lod(t, s, 0.5f, 0.5f, 5.f);
    EXPECT_NEAR(r.x, 1.f, 1e-3f);
    EXPECT_NEAR(r.y, 0.f, 1e-3f);
}

TEST_F(ShaderRuntimeTests, ComputeLod)
{
    SW_SRV t{};
    t.mips[0] = {256, 256, 1, 0, 0};
    t.mipLevels = 9;

    float lod = sw_compute_lod(t, 1.f/256.f, 0.f, 0.f, 1.f/256.f);
    EXPECT_NEAR(lod, 0.f, 0.1f);

    lod = sw_compute_lod(t, 2.f/256.f, 0.f, 0.f, 2.f/256.f);
    EXPECT_NEAR(lod, 1.f, 0.1f);

    lod = sw_compute_lod(t, 4.f/256.f, 0.f, 0.f, 4.f/256.f);
    EXPECT_NEAR(lod, 2.f, 0.1f);
}

TEST_F(ShaderRuntimeTests, SampleGrad)
{
    unsigned char mip0[2][2][4] = {
        {{255,0,0,255}, {255,0,0,255}},
        {{255,0,0,255}, {255,0,0,255}},
    };
    unsigned char mip1[1][1][4] = {{{0,255,0,255}}};

    unsigned char data[sizeof(mip0) + sizeof(mip1)];
    std::memcpy(data, mip0, sizeof(mip0));
    std::memcpy(data + sizeof(mip0), mip1, sizeof(mip1));

    SW_SRV t{};
    t.data = data;
    t.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    t.mipLevels = 2;
    t.mips[0] = {2, 2, 1, 2*4, 2*2*4};
    t.mips[1] = {1, 1, 1, 1*4, 1*4};
    t.mipOffsets[0] = 0;
    t.mipOffsets[1] = sizeof(mip0);

    SW_Sampler s{};
    s.filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    s.addressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    s.addressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    s.maxLOD = 1.f;

    SW_float4 r0 = sw_sample_2d_grad(t, s, 0.5f, 0.5f, 0.5f, 0.f, 0.f, 0.5f);
    EXPECT_NEAR(r0.x, 1.f, 1e-3f);

    SW_float4 r1 = sw_sample_2d_grad(t, s, 0.5f, 0.5f, 1.f, 0.f, 0.f, 1.f);
    EXPECT_NEAR(r1.x, 0.f, 1e-3f);
    EXPECT_NEAR(r1.y, 1.f, 1e-3f);
}

TEST_F(ShaderRuntimeTests, ResinfoFloatMip)
{
    SW_SRV t{};
    t.mipLevels = 3;
    t.mips[0] = {256, 128, 1, 0, 0};
    t.mips[1] = {128, 64, 1, 0, 0};
    t.mips[2] = {64, 32, 1, 0, 0};

    SW_float4 r0 = sw_resinfo_float_mip(t, 0);
    EXPECT_FLOAT_EQ(r0.x, 256.f);
    EXPECT_FLOAT_EQ(r0.y, 128.f);
    EXPECT_FLOAT_EQ(r0.w, 3.f);

    SW_float4 r2 = sw_resinfo_float_mip(t, 2);
    EXPECT_FLOAT_EQ(r2.x, 64.f);
    EXPECT_FLOAT_EQ(r2.y, 32.f);

    SW_float4 rClamp = sw_resinfo_float_mip(t, 99);
    EXPECT_FLOAT_EQ(rClamp.x, 64.f);
}

TEST_F(ShaderRuntimeTests, ResinfoUintMip)
{
    SW_SRV t{};
    t.mipLevels = 2;
    t.mips[0] = {512, 256, 1, 0, 0};
    t.mips[1] = {256, 128, 1, 0, 0};

    SW_float4 r1 = sw_resinfo_uint_mip(t, 1);
    EXPECT_EQ(sw_bits_uint(r1.x), 256u);
    EXPECT_EQ(sw_bits_uint(r1.y), 128u);
    EXPECT_EQ(sw_bits_uint(r1.w), 2u);
}

TEST_F(ShaderRuntimeTests, FetchTexel3dMip)
{
    float mip0[2][2][2] = {{{1,2},{3,4}},{{5,6},{7,8}}};
    float mip1[1][1][1] = {{{99}}};

    unsigned char data[sizeof(mip0) + sizeof(mip1)];
    std::memcpy(data, mip0, sizeof(mip0));
    std::memcpy(data + sizeof(mip0), mip1, sizeof(mip1));

    SW_SRV t{};
    t.data = data;
    t.format = DXGI_FORMAT_R32_FLOAT;
    t.mipLevels = 2;
    t.mips[0] = {2, 2, 2, 2*4, 2*2*4};
    t.mips[1] = {1, 1, 1, 1*4, 1*4};
    t.mipOffsets[0] = 0;
    t.mipOffsets[1] = sizeof(mip0);

    SW_float4 r0 = sw_fetch_texel_3d_mip(t, 0, 0, 0, 0);
    EXPECT_FLOAT_EQ(r0.x, 1.f);

    SW_float4 r0b = sw_fetch_texel_3d_mip(t, 1, 1, 1, 0);
    EXPECT_FLOAT_EQ(r0b.x, 8.f);

    SW_float4 r1 = sw_fetch_texel_3d_mip(t, 0, 0, 0, 1);
    EXPECT_FLOAT_EQ(r1.x, 99.f);
}

TEST_F(ShaderRuntimeTests, SampleLodBias)
{
    unsigned char mip0[1][1][4] = {{{255,0,0,255}}};
    unsigned char mip1[1][1][4] = {{{0,255,0,255}}};

    unsigned char data[sizeof(mip0) + sizeof(mip1)];
    std::memcpy(data, mip0, sizeof(mip0));
    std::memcpy(data + sizeof(mip0), mip1, sizeof(mip1));

    SW_SRV t{};
    t.data = data;
    t.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    t.mipLevels = 2;
    t.mips[0] = {1, 1, 1, 4, 4};
    t.mips[1] = {1, 1, 1, 4, 4};
    t.mipOffsets[0] = 0;
    t.mipOffsets[1] = sizeof(mip0);

    SW_Sampler s{};
    s.filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    s.addressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    s.addressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    s.mipLODBias = 1.f;
    s.maxLOD = 1.f;

    SW_float4 r = sw_sample_2d_lod(t, s, 0.5f, 0.5f, 0.f);
    EXPECT_NEAR(r.x, 0.f, 1e-3f);
    EXPECT_NEAR(r.y, 1.f, 1e-3f);
}

TEST_F(ShaderRuntimeTests, Bufinfo)
{
    SW_UAV u{};
    u.elementCount = 1024;

    SW_float4 r = sw_bufinfo(u);
    EXPECT_EQ(sw_bits_uint(r.x), 1024u);
}

TEST_F(ShaderRuntimeTests, UavStoreTyped_R8G8B8A8_UNORM)
{
    unsigned char pixels[4 * 4] = {};
    SW_UAV u{};
    u.data         = pixels;
    u.format       = DXGI_FORMAT_R8G8B8A8_UNORM;
    u.width        = 2;
    u.elementCount = 4;
    u.dimension    = D3D11_UAV_DIMENSION_TEXTURE2D;

    sw_uav_store_typed(u, 0, 0, {1.0f, 0.0f, 0.0f, 1.0f});
    sw_uav_store_typed(u, 1, 0, {0.0f, 1.0f, 0.0f, 1.0f});
    sw_uav_store_typed(u, 0, 1, {0.0f, 0.0f, 1.0f, 1.0f});
    sw_uav_store_typed(u, 1, 1, {0.5f, 0.25f, 0.75f, 1.0f});

    EXPECT_EQ(pixels[0], 255); EXPECT_EQ(pixels[1], 0);   EXPECT_EQ(pixels[2], 0);   EXPECT_EQ(pixels[3], 255);
    EXPECT_EQ(pixels[4], 0);   EXPECT_EQ(pixels[5], 255); EXPECT_EQ(pixels[6], 0);   EXPECT_EQ(pixels[7], 255);
    EXPECT_EQ(pixels[8], 0);   EXPECT_EQ(pixels[9], 0);   EXPECT_EQ(pixels[10], 255); EXPECT_EQ(pixels[11], 255);
    EXPECT_EQ(pixels[12], 127); EXPECT_EQ(pixels[13], 63); EXPECT_EQ(pixels[14], 191); EXPECT_EQ(pixels[15], 255);
}

TEST_F(ShaderRuntimeTests, UavStoreTyped_R8G8B8A8_UNORM_Clamps)
{
    unsigned char pixels[4] = {};
    SW_UAV u{};
    u.data         = pixels;
    u.format       = DXGI_FORMAT_R8G8B8A8_UNORM;
    u.width        = 1;
    u.elementCount = 1;
    u.dimension    = D3D11_UAV_DIMENSION_TEXTURE2D;

    sw_uav_store_typed(u, 0, 0, {2.0f, -1.0f, 0.0f, 1.0f});
    EXPECT_EQ(pixels[0], 255);
    EXPECT_EQ(pixels[1], 0);
    EXPECT_EQ(pixels[2], 0);
    EXPECT_EQ(pixels[3], 255);
}
