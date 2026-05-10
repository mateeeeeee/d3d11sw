#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "core/rasterizer/depth_stencil_util.h"
#include "core/rasterizer/blend_util.h"

using namespace d3dsw;

TEST(DrawPipeline, ApplyStencilOp_Keep)
{
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::Keep, 42, 0), 42);
}

TEST(DrawPipeline, ApplyStencilOp_Zero)
{
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::Zero, 42, 0), 0);
}

TEST(DrawPipeline, ApplyStencilOp_Replace)
{
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::Replace, 42, 99), 99);
}

TEST(DrawPipeline, ApplyStencilOp_IncrSat)
{
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::IncrSat, 254, 0), 255);
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::IncrSat, 255, 0), 255);
}

TEST(DrawPipeline, ApplyStencilOp_DecrSat)
{
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::DecrSat, 1, 0), 0);
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::DecrSat, 0, 0), 0);
}

TEST(DrawPipeline, ApplyStencilOp_Invert)
{
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::Invert, 0x00, 0), 0xFF);
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::Invert, 0xAA, 0), 0x55);
}

TEST(DrawPipeline, ApplyStencilOp_Incr_Wrap)
{
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::Incr, 255, 0), 0);
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::Incr, 100, 0), 101);
}

TEST(DrawPipeline, ApplyStencilOp_Decr_Wrap)
{
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::Decr, 0, 0), 255);
    EXPECT_EQ(ApplyStencilOp(SWStencilOp::Decr, 100, 0), 99);
}

TEST(DrawPipeline, CompareStencil_Always)
{
    EXPECT_TRUE(CompareStencil(SWComparison::Always, 0, 0));
    EXPECT_TRUE(CompareStencil(SWComparison::Always, 5, 200));
}

TEST(DrawPipeline, CompareStencil_Never)
{
    EXPECT_FALSE(CompareStencil(SWComparison::Never, 0, 0));
}

TEST(DrawPipeline, CompareStencil_Equal)
{
    EXPECT_TRUE(CompareStencil(SWComparison::Equal, 5, 5));
    EXPECT_FALSE(CompareStencil(SWComparison::Equal, 5, 6));
}

TEST(DrawPipeline, CompareStencil_Less)
{
    EXPECT_TRUE(CompareStencil(SWComparison::Less, 3, 5));
    EXPECT_FALSE(CompareStencil(SWComparison::Less, 5, 3));
}

TEST(DrawPipeline, CompareStencil_Greater)
{
    EXPECT_TRUE(CompareStencil(SWComparison::Greater, 5, 3));
    EXPECT_FALSE(CompareStencil(SWComparison::Greater, 3, 5));
}

TEST(DrawPipeline, CompareDepth_Less)
{
    EXPECT_TRUE(CompareDepth(SWComparison::Less, 0.3f, 0.5f));
    EXPECT_FALSE(CompareDepth(SWComparison::Less, 0.5f, 0.3f));
    EXPECT_FALSE(CompareDepth(SWComparison::Less, 0.5f, 0.5f));
}

TEST(DrawPipeline, CompareDepth_Greater)
{
    EXPECT_TRUE(CompareDepth(SWComparison::Greater, 0.7f, 0.3f));
    EXPECT_FALSE(CompareDepth(SWComparison::Greater, 0.3f, 0.7f));
}

TEST(DrawPipeline, CompareDepth_Always)
{
    EXPECT_TRUE(CompareDepth(SWComparison::Always, 0.f, 1.f));
}

TEST(DrawPipeline, CompareDepth_Never)
{
    EXPECT_FALSE(CompareDepth(SWComparison::Never, 0.f, 1.f));
}

TEST(DrawPipeline, StencilValueRoundtrip_D24S8)
{
    UINT8 buf[4] = {0, 0, 0, 0};
    WriteStencilValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf, 42);
    EXPECT_EQ(ReadStencilValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf), 42);

    WriteStencilValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf, 255);
    EXPECT_EQ(ReadStencilValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf), 255);
}

TEST(DrawPipeline, StencilValueRoundtrip_D32S8)
{
    UINT8 buf[8] = {};
    WriteStencilValue(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, buf, 99);
    EXPECT_EQ(ReadStencilValue(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, buf), 99);
}

TEST(DrawPipeline, DepthValueRoundtrip_D32)
{
    UINT8 buf[4] = {};
    WriteDepthValue(DXGI_FORMAT_D32_FLOAT, buf, 0.75f);
    EXPECT_FLOAT_EQ(ReadDepthValue(DXGI_FORMAT_D32_FLOAT, buf), 0.75f);
}

TEST(DrawPipeline, DepthValueRoundtrip_D24S8)
{
    UINT8 buf[4] = {};
    WriteDepthValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf, 0.5f);
    Float read = ReadDepthValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf);
    EXPECT_NEAR(read, 0.5f, 1.f / 16777215.f);
}

TEST(DrawPipeline, ApplyLogicOp_Clear)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::Clear, 0xAB, 0xCD), 0);
}

TEST(DrawPipeline, ApplyLogicOp_Set)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::Set, 0x00, 0x00), 0xFF);
}

TEST(DrawPipeline, ApplyLogicOp_Copy)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::Copy, 0xAB, 0xCD), 0xAB);
}

TEST(DrawPipeline, ApplyLogicOp_CopyInverted)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::CopyInverted, 0xAB, 0xCD), (UINT8)~0xAB);
}

TEST(DrawPipeline, ApplyLogicOp_Noop)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::Noop, 0xAB, 0xCD), 0xCD);
}

TEST(DrawPipeline, ApplyLogicOp_Invert)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::Invert, 0xAB, 0xCD), (UINT8)~0xCD);
}

TEST(DrawPipeline, ApplyLogicOp_And)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::And, 0xF0, 0x3C), (UINT8)(0xF0 & 0x3C));
}

TEST(DrawPipeline, ApplyLogicOp_Nand)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::Nand, 0xF0, 0x3C), (UINT8)~(0xF0 & 0x3C));
}

TEST(DrawPipeline, ApplyLogicOp_Or)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::Or, 0xF0, 0x0F), 0xFF);
}

TEST(DrawPipeline, ApplyLogicOp_Nor)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::Nor, 0xF0, 0x0F), 0x00);
}

TEST(DrawPipeline, ApplyLogicOp_Xor)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::Xor, 0xFF, 0xAA), 0x55);
}

TEST(DrawPipeline, ApplyLogicOp_Equiv)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::Equiv, 0xFF, 0xAA), (UINT8)~(0xFF ^ 0xAA));
}

TEST(DrawPipeline, ApplyLogicOp_AndReverse)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::AndReverse, 0xF0, 0x3C), (UINT8)(0xF0 & ~0x3C));
}

TEST(DrawPipeline, ApplyLogicOp_AndInverted)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::AndInverted, 0xF0, 0x3C), (UINT8)(~0xF0 & 0x3C));
}

TEST(DrawPipeline, ApplyLogicOp_OrReverse)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::OrReverse, 0xF0, 0x0C), (UINT8)(0xF0 | ~0x0C));
}

TEST(DrawPipeline, ApplyLogicOp_OrInverted)
{
    EXPECT_EQ(ApplyLogicOp(SWLogicOp::OrInverted, 0xF0, 0x0C), (UINT8)(~0xF0 | 0x0C));
}
