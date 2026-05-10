#include <gtest/gtest.h>
#include <d3d9.h>

#include "d3d9/context/draw_state.h"
#include "d3d9/context/pipeline_state_builder.h"
#include "d3d9/device/device.h"
#include "d3d9/translate/fvf.h"
#include "core/pipeline/sw_pipeline_state.h"

using namespace d3dsw;

namespace
{
    IDirect3DDevice9* MakeDevice()
    {
        IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
        D3DPRESENT_PARAMETERS pp = {};
        pp.BackBufferWidth  = 32;
        pp.BackBufferHeight = 32;
        pp.BackBufferFormat = D3DFMT_A8R8G8B8;
        pp.BackBufferCount  = 1;
        pp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
        pp.Windowed         = TRUE;
        IDirect3DDevice9* dev = nullptr;
        d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                          D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
        d3d->Release();
        return dev;
    }
}

// --- FVF → decl unit tests ----------------------------------------------

TEST(FVF, XYZ_SingleElement)
{
    std::vector<D3DVERTEXELEMENT9> decl;
    FVFToDeclaration(D3DFVF_XYZ, decl);
    ASSERT_EQ(decl.size(), 2u);  // 1 element + D3DDECL_END
    EXPECT_EQ(decl[0].Stream,     0);
    EXPECT_EQ(decl[0].Offset,     0);
    EXPECT_EQ(decl[0].Type,       D3DDECLTYPE_FLOAT3);
    EXPECT_EQ(decl[0].Usage,      D3DDECLUSAGE_POSITION);
    EXPECT_EQ(decl[0].UsageIndex, 0);
    EXPECT_EQ(decl[1].Stream,     0xFF);
    EXPECT_EQ(decl[1].Type,       D3DDECLTYPE_UNUSED);
}

TEST(FVF, XYZRHW_UsesPositionT)
{
    std::vector<D3DVERTEXELEMENT9> decl;
    FVFToDeclaration(D3DFVF_XYZRHW, decl);
    ASSERT_EQ(decl.size(), 2u);
    EXPECT_EQ(decl[0].Type,  D3DDECLTYPE_FLOAT4);
    EXPECT_EQ(decl[0].Usage, D3DDECLUSAGE_POSITIONT);
}

TEST(FVF, XYZW_UsesPositionFloat4)
{
    std::vector<D3DVERTEXELEMENT9> decl;
    FVFToDeclaration(D3DFVF_XYZW, decl);
    ASSERT_EQ(decl.size(), 2u);
    EXPECT_EQ(decl[0].Type,  D3DDECLTYPE_FLOAT4);
    EXPECT_EQ(decl[0].Usage, D3DDECLUSAGE_POSITION);
}

TEST(FVF, XYZ_DIFFUSE_TEX2_FullLayout)
{
    std::vector<D3DVERTEXELEMENT9> decl;
    FVFToDeclaration(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2, decl);
    ASSERT_EQ(decl.size(), 5u);   // pos + diffuse + tc0 + tc1 + END

    EXPECT_EQ(decl[0].Type,       D3DDECLTYPE_FLOAT3);
    EXPECT_EQ(decl[0].Usage,      D3DDECLUSAGE_POSITION);
    EXPECT_EQ(decl[0].Offset,     0);

    EXPECT_EQ(decl[1].Type,       D3DDECLTYPE_D3DCOLOR);
    EXPECT_EQ(decl[1].Usage,      D3DDECLUSAGE_COLOR);
    EXPECT_EQ(decl[1].UsageIndex, 0);
    EXPECT_EQ(decl[1].Offset,     12);

    EXPECT_EQ(decl[2].Type,       D3DDECLTYPE_FLOAT2);
    EXPECT_EQ(decl[2].Usage,      D3DDECLUSAGE_TEXCOORD);
    EXPECT_EQ(decl[2].UsageIndex, 0);
    EXPECT_EQ(decl[2].Offset,     16);

    EXPECT_EQ(decl[3].Type,       D3DDECLTYPE_FLOAT2);
    EXPECT_EQ(decl[3].Usage,      D3DDECLUSAGE_TEXCOORD);
    EXPECT_EQ(decl[3].UsageIndex, 1);
    EXPECT_EQ(decl[3].Offset,     24);
}

TEST(FVF, TexCoordSize_Overrides)
{
    // TexCoord 0 = FLOAT3, TexCoord 1 = FLOAT1
    const DWORD fvf = D3DFVF_XYZ | D3DFVF_TEX2 |
                      D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEXCOORDSIZE1(1);
    std::vector<D3DVERTEXELEMENT9> decl;
    FVFToDeclaration(fvf, decl);
    ASSERT_EQ(decl.size(), 4u);  // pos + tc0 + tc1 + END

    EXPECT_EQ(decl[1].Type,   D3DDECLTYPE_FLOAT3);
    EXPECT_EQ(decl[1].Offset, 12);

    EXPECT_EQ(decl[2].Type,   D3DDECLTYPE_FLOAT1);
    EXPECT_EQ(decl[2].Offset, 24);   // 12 (pos) + 12 (tc0 FLOAT3)
}

TEST(FVF, XYZ_NORMAL_DIFFUSE_SPECULAR_TEX1_OrderAndOffsets)
{
    const DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1;
    std::vector<D3DVERTEXELEMENT9> decl;
    FVFToDeclaration(fvf, decl);
    ASSERT_EQ(decl.size(), 6u);

    EXPECT_EQ(decl[0].Usage,      D3DDECLUSAGE_POSITION);
    EXPECT_EQ(decl[0].Offset,     0);

    EXPECT_EQ(decl[1].Usage,      D3DDECLUSAGE_NORMAL);
    EXPECT_EQ(decl[1].Offset,     12);

    EXPECT_EQ(decl[2].Usage,      D3DDECLUSAGE_COLOR);
    EXPECT_EQ(decl[2].UsageIndex, 0);  // DIFFUSE → COLOR0
    EXPECT_EQ(decl[2].Offset,     24);

    EXPECT_EQ(decl[3].Usage,      D3DDECLUSAGE_COLOR);
    EXPECT_EQ(decl[3].UsageIndex, 1);  // SPECULAR → COLOR1
    EXPECT_EQ(decl[3].Offset,     28);

    EXPECT_EQ(decl[4].Usage,      D3DDECLUSAGE_TEXCOORD);
    EXPECT_EQ(decl[4].Offset,     32);
}

TEST(FVF, XYZB1_AppendsBlendWeight)
{
    std::vector<D3DVERTEXELEMENT9> decl;
    FVFToDeclaration(D3DFVF_XYZB1, decl);
    ASSERT_EQ(decl.size(), 3u);   // pos + weight + END
    EXPECT_EQ(decl[0].Usage,  D3DDECLUSAGE_POSITION);
    EXPECT_EQ(decl[0].Type,   D3DDECLTYPE_FLOAT3);
    EXPECT_EQ(decl[1].Usage,  D3DDECLUSAGE_BLENDWEIGHT);
    EXPECT_EQ(decl[1].Type,   D3DDECLTYPE_FLOAT1);
}

TEST(FVF, XYZB3_ThreeWeightsAsFloat3)
{
    std::vector<D3DVERTEXELEMENT9> decl;
    FVFToDeclaration(D3DFVF_XYZB3, decl);
    ASSERT_EQ(decl.size(), 3u);
    EXPECT_EQ(decl[1].Usage,  D3DDECLUSAGE_BLENDWEIGHT);
    EXPECT_EQ(decl[1].Type,   D3DDECLTYPE_FLOAT3);
}

// --- Integration with BuildSWPipelineState ------------------------------

TEST(FVF, PipelineBuilder_UsesFVFWhenVertexDeclIsNull)
{
    IDirect3DDevice9* dev = MakeDevice();
    auto* devSW = static_cast<D3D9DeviceSW*>(dev);

    ASSERT_EQ(dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1), S_OK);

    D3D9SW_DRAW_STATE ds;
    devSW->SnapshotDrawState(ds, D3DPT_TRIANGLELIST);
    SWPipelineState sps;
    BuildSWPipelineState(ds, sps);

    ASSERT_EQ(sps.numInputElements, 3u);
    EXPECT_STREQ(sps.inputElements[0].semanticName, "POSITION");
    EXPECT_EQ(sps.inputElements[0].format, DXGI_FORMAT_R32G32B32_FLOAT);
    EXPECT_STREQ(sps.inputElements[1].semanticName, "COLOR");
    EXPECT_EQ(sps.inputElements[1].format, DXGI_FORMAT_B8G8R8A8_UNORM);
    EXPECT_STREQ(sps.inputElements[2].semanticName, "TEXCOORD");
    EXPECT_EQ(sps.inputElements[2].format, DXGI_FORMAT_R32G32_FLOAT);

    dev->Release();
}

TEST(FVF, PipelineBuilder_VertexDeclWinsOverFVF)
{
    IDirect3DDevice9* dev = MakeDevice();
    auto* devSW = static_cast<D3D9DeviceSW*>(dev);

    // Set both; the decl defines a single POSITION FLOAT4 element.
    D3DVERTEXELEMENT9 decl[] = {
        { 0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        D3DDECL_END()
    };
    IDirect3DVertexDeclaration9* vd = nullptr;
    ASSERT_EQ(dev->CreateVertexDeclaration(decl, &vd), S_OK);
    dev->SetVertexDeclaration(vd);
    dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);   // deliberately different

    D3D9SW_DRAW_STATE ds;
    devSW->SnapshotDrawState(ds, D3DPT_TRIANGLELIST);
    SWPipelineState sps;
    BuildSWPipelineState(ds, sps);

    // Decl wins: one element, FLOAT4.
    ASSERT_EQ(sps.numInputElements, 1u);
    EXPECT_EQ(sps.inputElements[0].format, DXGI_FORMAT_R32G32B32A32_FLOAT);

    vd->Release();
    dev->Release();
}
