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

TEST(FormatPacking, Roundtrip_R16G16B16A16_FLOAT)
{
    FLOAT rgba[4] = {1.0f, 0.5f, -0.25f, 100.0f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R16G16B16A16_FLOAT, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R16G16B16A16_FLOAT, packed, out);
    EXPECT_NEAR(out[0], 1.0f, 0.001f);
    EXPECT_NEAR(out[1], 0.5f, 0.001f);
    EXPECT_NEAR(out[2], -0.25f, 0.001f);
    EXPECT_NEAR(out[3], 100.0f, 0.1f);
}

TEST(FormatPacking, Roundtrip_R16G16_FLOAT)
{
    FLOAT rgba[4] = {2.5f, -1.0f, 0.f, 0.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R16G16_FLOAT, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R16G16_FLOAT, packed, out);
    EXPECT_NEAR(out[0], 2.5f, 0.001f);
    EXPECT_NEAR(out[1], -1.0f, 0.001f);
    EXPECT_FLOAT_EQ(out[2], 0.f);
    EXPECT_FLOAT_EQ(out[3], 0.f);
}

TEST(FormatPacking, Roundtrip_R16_FLOAT)
{
    FLOAT rgba[4] = {0.75f, 0.f, 0.f, 0.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R16_FLOAT, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R16_FLOAT, packed, out);
    EXPECT_NEAR(out[0], 0.75f, 0.001f);
}

TEST(FormatPacking, Roundtrip_R11G11B10_FLOAT)
{
    FLOAT rgba[4] = {1.0f, 0.5f, 2.0f, 0.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R11G11B10_FLOAT, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R11G11B10_FLOAT, packed, out);
    EXPECT_NEAR(out[0], 1.0f, 0.02f);
    EXPECT_NEAR(out[1], 0.5f, 0.02f);
    EXPECT_NEAR(out[2], 2.0f, 0.1f);
    EXPECT_FLOAT_EQ(out[3], 1.0f);
}

TEST(FormatPacking, Roundtrip_R10G10B10A2_UNORM)
{
    FLOAT rgba[4] = {1.0f, 0.5f, 0.25f, 1.0f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R10G10B10A2_UNORM, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R10G10B10A2_UNORM, packed, out);
    EXPECT_NEAR(out[0], 1.0f, 1.f / 1023.f + 0.001f);
    EXPECT_NEAR(out[1], 0.5f, 1.f / 1023.f + 0.001f);
    EXPECT_NEAR(out[2], 0.25f, 1.f / 1023.f + 0.001f);
    EXPECT_NEAR(out[3], 1.0f, 1.f / 3.f + 0.001f);
}

TEST(FormatPacking, Roundtrip_R10G10B10A2_UINT)
{
    FLOAT rgba[4] = {512.f, 100.f, 1023.f, 3.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R10G10B10A2_UINT, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R10G10B10A2_UINT, packed, out);
    EXPECT_FLOAT_EQ(out[0], 512.f);
    EXPECT_FLOAT_EQ(out[1], 100.f);
    EXPECT_FLOAT_EQ(out[2], 1023.f);
    EXPECT_FLOAT_EQ(out[3], 3.f);
}

TEST(FormatPacking, Roundtrip_R8G8B8A8_SNORM)
{
    FLOAT rgba[4] = {1.0f, -1.0f, 0.5f, -0.5f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R8G8B8A8_SNORM, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R8G8B8A8_SNORM, packed, out);
    EXPECT_NEAR(out[0], 1.0f, 1.f / 127.f + 0.01f);
    EXPECT_NEAR(out[1], -1.0f, 1.f / 127.f + 0.01f);
    EXPECT_NEAR(out[2], 0.5f, 1.f / 127.f + 0.01f);
    EXPECT_NEAR(out[3], -0.5f, 1.f / 127.f + 0.01f);
}

TEST(FormatPacking, Roundtrip_R8G8B8A8_UINT)
{
    FLOAT rgba[4] = {10.f, 20.f, 200.f, 255.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R8G8B8A8_UINT, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R8G8B8A8_UINT, packed, out);
    EXPECT_FLOAT_EQ(out[0], 10.f);
    EXPECT_FLOAT_EQ(out[1], 20.f);
    EXPECT_FLOAT_EQ(out[2], 200.f);
    EXPECT_FLOAT_EQ(out[3], 255.f);
}

TEST(FormatPacking, Roundtrip_R8G8B8A8_SINT)
{
    FLOAT rgba[4] = {-10.f, 20.f, -100.f, 127.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R8G8B8A8_SINT, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R8G8B8A8_SINT, packed, out);
    EXPECT_FLOAT_EQ(out[0], -10.f);
    EXPECT_FLOAT_EQ(out[1], 20.f);
    EXPECT_FLOAT_EQ(out[2], -100.f);
    EXPECT_FLOAT_EQ(out[3], 127.f);
}

TEST(FormatPacking, Roundtrip_R8G8_UNORM)
{
    FLOAT rgba[4] = {0.75f, 0.25f, 0.f, 0.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R8G8_UNORM, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R8G8_UNORM, packed, out);
    EXPECT_NEAR(out[0], 0.75f, 1.f / 255.f + 0.001f);
    EXPECT_NEAR(out[1], 0.25f, 1.f / 255.f + 0.001f);
}

TEST(FormatPacking, Roundtrip_R8_UNORM)
{
    FLOAT rgba[4] = {0.5f, 0.f, 0.f, 0.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R8_UNORM, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R8_UNORM, packed, out);
    EXPECT_NEAR(out[0], 0.5f, 1.f / 255.f + 0.001f);
}

TEST(FormatPacking, Roundtrip_R32G32B32_FLOAT)
{
    FLOAT rgba[4] = {1.5f, 2.5f, 3.5f, 0.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R32G32B32_FLOAT, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R32G32B32_FLOAT, packed, out);
    EXPECT_FLOAT_EQ(out[0], 1.5f);
    EXPECT_FLOAT_EQ(out[1], 2.5f);
    EXPECT_FLOAT_EQ(out[2], 3.5f);
}

TEST(FormatPacking, Roundtrip_R32G32B32A32_UINT)
{
    FLOAT rgba[4] = {1.f, 200.f, 50000.f, 4000000.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R32G32B32A32_UINT, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R32G32B32A32_UINT, packed, out);
    EXPECT_FLOAT_EQ(out[0], 1.f);
    EXPECT_FLOAT_EQ(out[1], 200.f);
    EXPECT_FLOAT_EQ(out[2], 50000.f);
    EXPECT_FLOAT_EQ(out[3], 4000000.f);
}

TEST(FormatPacking, Roundtrip_R16G16_UNORM)
{
    FLOAT rgba[4] = {0.75f, 0.25f, 0.f, 0.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R16G16_UNORM, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R16G16_UNORM, packed, out);
    EXPECT_NEAR(out[0], 0.75f, 1.f / 65535.f + 0.0001f);
    EXPECT_NEAR(out[1], 0.25f, 1.f / 65535.f + 0.0001f);
}

TEST(FormatPacking, Roundtrip_R16_UNORM)
{
    FLOAT rgba[4] = {0.5f, 0.f, 0.f, 0.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R16_UNORM, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R16_UNORM, packed, out);
    EXPECT_NEAR(out[0], 0.5f, 1.f / 65535.f + 0.0001f);
}

TEST(FormatPacking, Roundtrip_R32_UINT)
{
    FLOAT rgba[4] = {42.f, 0.f, 0.f, 0.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R32_UINT, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R32_UINT, packed, out);
    EXPECT_FLOAT_EQ(out[0], 42.f);
}

TEST(FormatPacking, Roundtrip_R32G32_UINT)
{
    FLOAT rgba[4] = {100.f, 200.f, 0.f, 0.f};
    UINT8 packed[16] = {};
    PackRTVColor(DXGI_FORMAT_R32G32_UINT, rgba, packed);
    FLOAT out[4] = {};
    UnpackColor(DXGI_FORMAT_R32G32_UINT, packed, out);
    EXPECT_FLOAT_EQ(out[0], 100.f);
    EXPECT_FLOAT_EQ(out[1], 200.f);
}
