#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "context/depth_stencil_util.h"
#include "context/blend_util.h"

using namespace d3d11sw;

TEST(DrawPipeline, ApplyStencilOp_Keep)
{
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_KEEP, 42, 0), 42);
}

TEST(DrawPipeline, ApplyStencilOp_Zero)
{
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_ZERO, 42, 0), 0);
}

TEST(DrawPipeline, ApplyStencilOp_Replace)
{
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_REPLACE, 42, 99), 99);
}

TEST(DrawPipeline, ApplyStencilOp_IncrSat)
{
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_INCR_SAT, 254, 0), 255);
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_INCR_SAT, 255, 0), 255);
}

TEST(DrawPipeline, ApplyStencilOp_DecrSat)
{
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_DECR_SAT, 1, 0), 0);
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_DECR_SAT, 0, 0), 0);
}

TEST(DrawPipeline, ApplyStencilOp_Invert)
{
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_INVERT, 0x00, 0), 0xFF);
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_INVERT, 0xAA, 0), 0x55);
}

TEST(DrawPipeline, ApplyStencilOp_Incr_Wrap)
{
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_INCR, 255, 0), 0);
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_INCR, 100, 0), 101);
}

TEST(DrawPipeline, ApplyStencilOp_Decr_Wrap)
{
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_DECR, 0, 0), 255);
    EXPECT_EQ(ApplyStencilOp(D3D11_STENCIL_OP_DECR, 100, 0), 99);
}

TEST(DrawPipeline, CompareStencil_Always)
{
    EXPECT_TRUE(CompareStencil(D3D11_COMPARISON_ALWAYS, 0, 0));
    EXPECT_TRUE(CompareStencil(D3D11_COMPARISON_ALWAYS, 5, 200));
}

TEST(DrawPipeline, CompareStencil_Never)
{
    EXPECT_FALSE(CompareStencil(D3D11_COMPARISON_NEVER, 0, 0));
}

TEST(DrawPipeline, CompareStencil_Equal)
{
    EXPECT_TRUE(CompareStencil(D3D11_COMPARISON_EQUAL, 5, 5));
    EXPECT_FALSE(CompareStencil(D3D11_COMPARISON_EQUAL, 5, 6));
}

TEST(DrawPipeline, CompareStencil_Less)
{
    EXPECT_TRUE(CompareStencil(D3D11_COMPARISON_LESS, 3, 5));
    EXPECT_FALSE(CompareStencil(D3D11_COMPARISON_LESS, 5, 3));
}

TEST(DrawPipeline, CompareStencil_Greater)
{
    EXPECT_TRUE(CompareStencil(D3D11_COMPARISON_GREATER, 5, 3));
    EXPECT_FALSE(CompareStencil(D3D11_COMPARISON_GREATER, 3, 5));
}

TEST(DrawPipeline, CompareDepth_Less)
{
    EXPECT_TRUE(CompareDepth(D3D11_COMPARISON_LESS, 0.3f, 0.5f));
    EXPECT_FALSE(CompareDepth(D3D11_COMPARISON_LESS, 0.5f, 0.3f));
    EXPECT_FALSE(CompareDepth(D3D11_COMPARISON_LESS, 0.5f, 0.5f));
}

TEST(DrawPipeline, CompareDepth_Greater)
{
    EXPECT_TRUE(CompareDepth(D3D11_COMPARISON_GREATER, 0.7f, 0.3f));
    EXPECT_FALSE(CompareDepth(D3D11_COMPARISON_GREATER, 0.3f, 0.7f));
}

TEST(DrawPipeline, CompareDepth_Always)
{
    EXPECT_TRUE(CompareDepth(D3D11_COMPARISON_ALWAYS, 0.f, 1.f));
}

TEST(DrawPipeline, CompareDepth_Never)
{
    EXPECT_FALSE(CompareDepth(D3D11_COMPARISON_NEVER, 0.f, 1.f));
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
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_CLEAR, 0xAB, 0xCD), 0);
}

TEST(DrawPipeline, ApplyLogicOp_Set)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_SET, 0x00, 0x00), 0xFF);
}

TEST(DrawPipeline, ApplyLogicOp_Copy)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_COPY, 0xAB, 0xCD), 0xAB);
}

TEST(DrawPipeline, ApplyLogicOp_CopyInverted)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_COPY_INVERTED, 0xAB, 0xCD), (UINT8)~0xAB);
}

TEST(DrawPipeline, ApplyLogicOp_Noop)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_NOOP, 0xAB, 0xCD), 0xCD);
}

TEST(DrawPipeline, ApplyLogicOp_Invert)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_INVERT, 0xAB, 0xCD), (UINT8)~0xCD);
}

TEST(DrawPipeline, ApplyLogicOp_And)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_AND, 0xF0, 0x3C), (UINT8)(0xF0 & 0x3C));
}

TEST(DrawPipeline, ApplyLogicOp_Nand)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_NAND, 0xF0, 0x3C), (UINT8)~(0xF0 & 0x3C));
}

TEST(DrawPipeline, ApplyLogicOp_Or)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_OR, 0xF0, 0x0F), 0xFF);
}

TEST(DrawPipeline, ApplyLogicOp_Nor)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_NOR, 0xF0, 0x0F), 0x00);
}

TEST(DrawPipeline, ApplyLogicOp_Xor)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_XOR, 0xFF, 0xAA), 0x55);
}

TEST(DrawPipeline, ApplyLogicOp_Equiv)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_EQUIV, 0xFF, 0xAA), (UINT8)~(0xFF ^ 0xAA));
}

TEST(DrawPipeline, ApplyLogicOp_AndReverse)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_AND_REVERSE, 0xF0, 0x3C), (UINT8)(0xF0 & ~0x3C));
}

TEST(DrawPipeline, ApplyLogicOp_AndInverted)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_AND_INVERTED, 0xF0, 0x3C), (UINT8)(~0xF0 & 0x3C));
}

TEST(DrawPipeline, ApplyLogicOp_OrReverse)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_OR_REVERSE, 0xF0, 0x0C), (UINT8)(0xF0 | ~0x0C));
}

TEST(DrawPipeline, ApplyLogicOp_OrInverted)
{
    EXPECT_EQ(ApplyLogicOp(D3D11_LOGIC_OP_OR_INVERTED, 0xF0, 0x0C), (UINT8)(~0xF0 | 0x0C));
}
