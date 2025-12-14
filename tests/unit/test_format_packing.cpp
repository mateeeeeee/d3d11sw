#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "context/context_util.h"
#include <cstring>

using namespace d3d11sw;

TEST(FormatPacking, PackRTVColor_R8G8B8A8_UNORM)
{
    FLOAT rgba[4] = {1.f, 0.5f, 0.f, 1.f};
    UINT8 out[16] = {};
    PackRTVColor(DXGI_FORMAT_R8G8B8A8_UNORM, rgba, out);
    EXPECT_EQ(out[0], 255);
    EXPECT_EQ(out[1], 128);
    EXPECT_EQ(out[2], 0);
    EXPECT_EQ(out[3], 255);
}

TEST(FormatPacking, PackRTVColor_B8G8R8A8_UNORM)
{
    FLOAT rgba[4] = {1.f, 0.f, 0.5f, 1.f};
    UINT8 out[16] = {};
    PackRTVColor(DXGI_FORMAT_B8G8R8A8_UNORM, rgba, out);
    EXPECT_EQ(out[0], 128);
    EXPECT_EQ(out[1], 0);
    EXPECT_EQ(out[2], 255);
    EXPECT_EQ(out[3], 255);
}

TEST(FormatPacking, PackRTVColor_R32_FLOAT)
{
    FLOAT rgba[4] = {3.14f, 0.f, 0.f, 0.f};
    UINT8 out[16] = {};
    PackRTVColor(DXGI_FORMAT_R32_FLOAT, rgba, out);
    float val;
    std::memcpy(&val, out, 4);
    EXPECT_FLOAT_EQ(val, 3.14f);
}

TEST(FormatPacking, PackRTVColor_R32G32B32A32_FLOAT)
{
    FLOAT rgba[4] = {1.f, 2.f, 3.f, 4.f};
    UINT8 out[16] = {};
    PackRTVColor(DXGI_FORMAT_R32G32B32A32_FLOAT, rgba, out);
    float vals[4];
    std::memcpy(vals, out, 16);
    EXPECT_FLOAT_EQ(vals[0], 1.f);
    EXPECT_FLOAT_EQ(vals[1], 2.f);
    EXPECT_FLOAT_EQ(vals[2], 3.f);
    EXPECT_FLOAT_EQ(vals[3], 4.f);
}

TEST(FormatPacking, PackRTVColor_R16G16B16A16_UNORM)
{
    FLOAT rgba[4] = {1.f, 0.5f, 0.f, 1.f};
    UINT8 out[16] = {};
    PackRTVColor(DXGI_FORMAT_R16G16B16A16_UNORM, rgba, out);
    UINT16 vals[4];
    std::memcpy(vals, out, 8);
    EXPECT_EQ(vals[0], 65535);
    EXPECT_NEAR(vals[1], 32768, 1);
    EXPECT_EQ(vals[2], 0);
    EXPECT_EQ(vals[3], 65535);
}

TEST(FormatPacking, PackRTVColor_R32_UINT)
{
    FLOAT rgba[4] = {42.f, 0.f, 0.f, 0.f};
    UINT8 out[16] = {};
    PackRTVColor(DXGI_FORMAT_R32_UINT, rgba, out);
    UINT val;
    std::memcpy(&val, out, 4);
    EXPECT_EQ(val, 42u);
}

TEST(FormatPacking, PackUAVUint_R32_UINT)
{
    UINT values[4] = {123, 0, 0, 0};
    UINT8 out[16] = {};
    PackUAVUint(DXGI_FORMAT_R32_UINT, values, out);
    UINT val;
    std::memcpy(&val, out, 4);
    EXPECT_EQ(val, 123u);
}

TEST(FormatPacking, PackUAVUint_R32G32B32A32_UINT)
{
    UINT values[4] = {1, 2, 3, 4};
    UINT8 out[16] = {};
    PackUAVUint(DXGI_FORMAT_R32G32B32A32_UINT, values, out);
    UINT vals[4];
    std::memcpy(vals, out, 16);
    EXPECT_EQ(vals[0], 1u);
    EXPECT_EQ(vals[1], 2u);
    EXPECT_EQ(vals[2], 3u);
    EXPECT_EQ(vals[3], 4u);
}

TEST(FormatPacking, PackUAVUint_R16G16B16A16_UINT)
{
    UINT values[4] = {100, 200, 300, 400};
    UINT8 out[16] = {};
    PackUAVUint(DXGI_FORMAT_R16G16B16A16_UINT, values, out);
    UINT16 vals[4];
    std::memcpy(vals, out, 8);
    EXPECT_EQ(vals[0], 100);
    EXPECT_EQ(vals[1], 200);
    EXPECT_EQ(vals[2], 300);
    EXPECT_EQ(vals[3], 400);
}

TEST(FormatPacking, PackUAVUint_R8G8B8A8_UINT)
{
    UINT values[4] = {10, 20, 30, 40};
    UINT8 out[16] = {};
    PackUAVUint(DXGI_FORMAT_R8G8B8A8_UINT, values, out);
    EXPECT_EQ(out[0], 10);
    EXPECT_EQ(out[1], 20);
    EXPECT_EQ(out[2], 30);
    EXPECT_EQ(out[3], 40);
}
