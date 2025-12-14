#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "context/rasterizer.h"

using namespace d3d11sw;

TEST(DrawPipeline, ApplyStencilOp_Keep)
{
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_KEEP, 42, 0), 42);
}

TEST(DrawPipeline, ApplyStencilOp_Zero)
{
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_ZERO, 42, 0), 0);
}

TEST(DrawPipeline, ApplyStencilOp_Replace)
{
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_REPLACE, 42, 99), 99);
}

TEST(DrawPipeline, ApplyStencilOp_IncrSat)
{
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_INCR_SAT, 254, 0), 255);
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_INCR_SAT, 255, 0), 255);
}

TEST(DrawPipeline, ApplyStencilOp_DecrSat)
{
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_DECR_SAT, 1, 0), 0);
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_DECR_SAT, 0, 0), 0);
}

TEST(DrawPipeline, ApplyStencilOp_Invert)
{
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_INVERT, 0x00, 0), 0xFF);
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_INVERT, 0xAA, 0), 0x55);
}

TEST(DrawPipeline, ApplyStencilOp_Incr_Wrap)
{
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_INCR, 255, 0), 0);
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_INCR, 100, 0), 101);
}

TEST(DrawPipeline, ApplyStencilOp_Decr_Wrap)
{
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_DECR, 0, 0), 255);
    EXPECT_EQ(SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP_DECR, 100, 0), 99);
}

TEST(DrawPipeline, CompareStencil_Always)
{
    EXPECT_TRUE(SWRasterizer::CompareStencil(D3D11_COMPARISON_ALWAYS, 0, 0));
    EXPECT_TRUE(SWRasterizer::CompareStencil(D3D11_COMPARISON_ALWAYS, 5, 200));
}

TEST(DrawPipeline, CompareStencil_Never)
{
    EXPECT_FALSE(SWRasterizer::CompareStencil(D3D11_COMPARISON_NEVER, 0, 0));
}

TEST(DrawPipeline, CompareStencil_Equal)
{
    EXPECT_TRUE(SWRasterizer::CompareStencil(D3D11_COMPARISON_EQUAL, 5, 5));
    EXPECT_FALSE(SWRasterizer::CompareStencil(D3D11_COMPARISON_EQUAL, 5, 6));
}

TEST(DrawPipeline, CompareStencil_Less)
{
    EXPECT_TRUE(SWRasterizer::CompareStencil(D3D11_COMPARISON_LESS, 3, 5));
    EXPECT_FALSE(SWRasterizer::CompareStencil(D3D11_COMPARISON_LESS, 5, 3));
}

TEST(DrawPipeline, CompareStencil_Greater)
{
    EXPECT_TRUE(SWRasterizer::CompareStencil(D3D11_COMPARISON_GREATER, 5, 3));
    EXPECT_FALSE(SWRasterizer::CompareStencil(D3D11_COMPARISON_GREATER, 3, 5));
}

TEST(DrawPipeline, CompareDepth_Less)
{
    EXPECT_TRUE(SWRasterizer::CompareDepth(D3D11_COMPARISON_LESS, 0.3f, 0.5f));
    EXPECT_FALSE(SWRasterizer::CompareDepth(D3D11_COMPARISON_LESS, 0.5f, 0.3f));
    EXPECT_FALSE(SWRasterizer::CompareDepth(D3D11_COMPARISON_LESS, 0.5f, 0.5f));
}

TEST(DrawPipeline, CompareDepth_Greater)
{
    EXPECT_TRUE(SWRasterizer::CompareDepth(D3D11_COMPARISON_GREATER, 0.7f, 0.3f));
    EXPECT_FALSE(SWRasterizer::CompareDepth(D3D11_COMPARISON_GREATER, 0.3f, 0.7f));
}

TEST(DrawPipeline, CompareDepth_Always)
{
    EXPECT_TRUE(SWRasterizer::CompareDepth(D3D11_COMPARISON_ALWAYS, 0.f, 1.f));
}

TEST(DrawPipeline, CompareDepth_Never)
{
    EXPECT_FALSE(SWRasterizer::CompareDepth(D3D11_COMPARISON_NEVER, 0.f, 1.f));
}

TEST(DrawPipeline, StencilValueRoundtrip_D24S8)
{
    UINT8 buf[4] = {0, 0, 0, 0};
    SWRasterizer::WriteStencilValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf, 42);
    EXPECT_EQ(SWRasterizer::ReadStencilValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf), 42);

    SWRasterizer::WriteStencilValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf, 255);
    EXPECT_EQ(SWRasterizer::ReadStencilValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf), 255);
}

TEST(DrawPipeline, StencilValueRoundtrip_D32S8)
{
    UINT8 buf[8] = {};
    SWRasterizer::WriteStencilValue(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, buf, 99);
    EXPECT_EQ(SWRasterizer::ReadStencilValue(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, buf), 99);
}

TEST(DrawPipeline, DepthValueRoundtrip_D32)
{
    UINT8 buf[4] = {};
    SWRasterizer::WriteDepthValue(DXGI_FORMAT_D32_FLOAT, buf, 0.75f);
    EXPECT_FLOAT_EQ(SWRasterizer::ReadDepthValue(DXGI_FORMAT_D32_FLOAT, buf), 0.75f);
}

TEST(DrawPipeline, DepthValueRoundtrip_D24S8)
{
    UINT8 buf[4] = {};
    SWRasterizer::WriteDepthValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf, 0.5f);
    Float read = SWRasterizer::ReadDepthValue(DXGI_FORMAT_D24_UNORM_S8_UINT, buf);
    EXPECT_NEAR(read, 0.5f, 1.f / 16777215.f);
}
