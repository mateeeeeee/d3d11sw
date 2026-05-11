#include <gtest/gtest.h>
#include <d3d9.h>
#include <cstring>
#include "d3d9/device/device.h"
#include "d3d9/context/draw_state.h"
#include "d3d9/context/pipeline_state_builder.h"
#include "d3d9/resources/vertex_buffer.h"
#include "d3d9/resources/index_buffer.h"
#include "d3d9/resources/surface.h"
#include "core/pipeline/sw_pipeline_state.h"

using namespace d3dsw;

namespace
{
    struct Env
    {
        IDirect3D9*       d3d = nullptr;
        IDirect3DDevice9* dev = nullptr;
        UINT              w   = 64;
        UINT              h   = 32;

        Env()
        {
            d3d = Direct3DCreate9(D3D_SDK_VERSION);
            D3DPRESENT_PARAMETERS pp = {};
            pp.BackBufferWidth  = w;
            pp.BackBufferHeight = h;
            pp.BackBufferFormat = D3DFMT_A8R8G8B8;
            pp.BackBufferCount  = 1;
            pp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
            pp.Windowed         = TRUE;
            d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                              D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
        }

        ~Env()
        {
            if (dev) { dev->Release(); }
            if (d3d) { d3d->Release(); }
        }

        D3D9DeviceSW* Dev() const { return static_cast<D3D9DeviceSW*>(dev); }
    };
}

TEST(D3D9DrawState, Topology_Mapping)
{
    Env e;
    D3D9SW_DRAW_STATE ds;
    SWPipelineState sps;

    const D3DPRIMITIVETYPE primTypes[] = {
        D3DPT_POINTLIST, D3DPT_LINELIST, D3DPT_LINESTRIP,
        D3DPT_TRIANGLELIST, D3DPT_TRIANGLESTRIP
    };
    const SWTopology expected[] = {
        SWTopology::PointList, SWTopology::LineList, SWTopology::LineStrip,
        SWTopology::TriangleList, SWTopology::TriangleStrip
    };
    for (int i = 0; i < 5; ++i)
    {
        e.Dev()->SnapshotDrawState(ds, primTypes[i]);
        BuildSWPipelineState(ds, sps);
        EXPECT_EQ(sps.topology, expected[i]);
    }

    // TRIANGLEFAN now maps correctly.
    e.Dev()->SnapshotDrawState(ds, D3DPT_TRIANGLEFAN);
    BuildSWPipelineState(ds, sps);
    EXPECT_EQ(sps.topology, SWTopology::TriangleFan);
}

TEST(D3D9DrawState, VertexDeclaration_BecomesInputElements)
{
    Env e;

    D3DVERTEXELEMENT9 decl[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        { 1,  0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
        D3DDECL_END()
    };
    IDirect3DVertexDeclaration9* vd = nullptr;
    ASSERT_EQ(e.dev->CreateVertexDeclaration(decl, &vd), S_OK);
    e.dev->SetVertexDeclaration(vd);

    D3D9SW_DRAW_STATE ds;
    e.Dev()->SnapshotDrawState(ds, D3DPT_TRIANGLELIST);
    SWPipelineState sps;
    BuildSWPipelineState(ds, sps);

    ASSERT_EQ(sps.numInputElements, 3u);

    EXPECT_STREQ(sps.inputElements[0].semanticName, "POSITION");
    EXPECT_EQ(sps.inputElements[0].semanticIndex,     0u);
    EXPECT_EQ(sps.inputElements[0].format,            DXGI_FORMAT_R32G32B32_FLOAT);
    EXPECT_EQ(sps.inputElements[0].inputSlot,         0u);
    EXPECT_EQ(sps.inputElements[0].alignedByteOffset, 0u);

    EXPECT_STREQ(sps.inputElements[1].semanticName, "TEXCOORD");
    EXPECT_EQ(sps.inputElements[1].format,          DXGI_FORMAT_R32G32_FLOAT);
    EXPECT_EQ(sps.inputElements[1].alignedByteOffset, 12u);

    EXPECT_STREQ(sps.inputElements[2].semanticName, "COLOR");
    EXPECT_EQ(sps.inputElements[2].format,          DXGI_FORMAT_B8G8R8A8_UNORM);
    EXPECT_EQ(sps.inputElements[2].inputSlot,       1u);

    vd->Release();
}

TEST(D3D9DrawState, StreamSource_DataOffsetFoldedIn)
{
    Env e;

    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(e.dev->CreateVertexBuffer(128, 0, 0, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);

    const UINT offset = 16;
    const UINT stride = 20;
    e.dev->SetStreamSource(2, vb, offset, stride);

    D3D9SW_DRAW_STATE ds;
    e.Dev()->SnapshotDrawState(ds, D3DPT_TRIANGLELIST);
    SWPipelineState sps;
    BuildSWPipelineState(ds, sps);

    auto* vbSW = static_cast<D3D9VertexBufferSW*>(vb);
    EXPECT_EQ(sps.vertexBuffers[2].data,   vbSW->DataPtr() + offset);
    EXPECT_EQ(sps.vertexBuffers[2].stride, stride);
    EXPECT_EQ(sps.vertexBuffers[2].offset, 0u);

    // Unbound slots are null.
    EXPECT_EQ(sps.vertexBuffers[0].data,   nullptr);
    EXPECT_EQ(sps.vertexBuffers[0].stride, 0u);

    vb->Release();
}

TEST(D3D9DrawState, IndexBuffer_Format16Or32)
{
    Env e;

    IDirect3DIndexBuffer9* ib16 = nullptr;
    ASSERT_EQ(e.dev->CreateIndexBuffer(64, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib16, nullptr), S_OK);
    e.dev->SetIndices(ib16);

    D3D9SW_DRAW_STATE ds;
    e.Dev()->SnapshotDrawState(ds, D3DPT_TRIANGLELIST);
    SWPipelineState sps;
    BuildSWPipelineState(ds, sps);
    EXPECT_EQ(sps.indexBuffer.format, DXGI_FORMAT_R16_UINT);
    EXPECT_EQ(sps.indexBuffer.data,   static_cast<D3D9IndexBufferSW*>(ib16)->DataPtr());

    IDirect3DIndexBuffer9* ib32 = nullptr;
    ASSERT_EQ(e.dev->CreateIndexBuffer(128, 0, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &ib32, nullptr), S_OK);
    e.dev->SetIndices(ib32);
    e.Dev()->SnapshotDrawState(ds, D3DPT_TRIANGLELIST);
    BuildSWPipelineState(ds, sps);
    EXPECT_EQ(sps.indexBuffer.format, DXGI_FORMAT_R32_UINT);

    ib16->Release();
    ib32->Release();
}

TEST(D3D9DrawState, RenderTarget_DefaultBackbufferPopulated)
{
    Env e;

    D3D9SW_DRAW_STATE ds;
    e.Dev()->SnapshotDrawState(ds, D3DPT_TRIANGLELIST);
    SWPipelineState sps;
    BuildSWPipelineState(ds, sps);

    ASSERT_EQ(sps.numRenderTargets, 1u);
    const auto& rt = sps.renderTargets[0];
    EXPECT_EQ(rt.format,    DXGI_FORMAT_B8G8R8A8_UNORM);  // from D3DFMT_A8R8G8B8
    EXPECT_EQ(rt.pixStride, 4u);
    EXPECT_EQ(rt.rowPitch,  e.w * 4u);
    EXPECT_EQ(rt.slicePitch, e.w * 4u * e.h);
    EXPECT_NE(rt.data,      nullptr);
}

TEST(D3D9DrawState, Viewport_DefaultsToBackbufferSize)
{
    Env e;

    D3D9SW_DRAW_STATE ds;
    e.Dev()->SnapshotDrawState(ds, D3DPT_TRIANGLELIST);
    SWPipelineState sps;
    BuildSWPipelineState(ds, sps);

    ASSERT_EQ(sps.numViewports, 1u);
    EXPECT_FLOAT_EQ(sps.viewports[0].topLeftX, 0.f);
    EXPECT_FLOAT_EQ(sps.viewports[0].topLeftY, 0.f);
    EXPECT_FLOAT_EQ(sps.viewports[0].width,    static_cast<Float>(e.w));
    EXPECT_FLOAT_EQ(sps.viewports[0].height,   static_cast<Float>(e.h));
    EXPECT_FLOAT_EQ(sps.viewports[0].minDepth, 0.f);
    EXPECT_FLOAT_EQ(sps.viewports[0].maxDepth, 1.f);
}

TEST(D3D9DrawState, Viewport_Override)
{
    Env e;

    D3DVIEWPORT9 vp = { 10, 20, 30, 40, 0.25f, 0.75f };
    ASSERT_EQ(e.dev->SetViewport(&vp), S_OK);

    D3D9SW_DRAW_STATE ds;
    e.Dev()->SnapshotDrawState(ds, D3DPT_TRIANGLELIST);
    SWPipelineState sps;
    BuildSWPipelineState(ds, sps);

    EXPECT_FLOAT_EQ(sps.viewports[0].topLeftX, 10.f);
    EXPECT_FLOAT_EQ(sps.viewports[0].topLeftY, 20.f);
    EXPECT_FLOAT_EQ(sps.viewports[0].width,    30.f);
    EXPECT_FLOAT_EQ(sps.viewports[0].height,   40.f);
    EXPECT_FLOAT_EQ(sps.viewports[0].minDepth, 0.25f);
    EXPECT_FLOAT_EQ(sps.viewports[0].maxDepth, 0.75f);
}

TEST(D3D9DrawState, MultipleRenderTargets_CountReflectsHighestSet)
{
    Env e;

    IDirect3DSurface9* rt2 = nullptr;
    ASSERT_EQ(e.dev->CreateRenderTarget(16, 16, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &rt2, nullptr), S_OK);
    ASSERT_EQ(e.dev->SetRenderTarget(2, rt2), S_OK);

    D3D9SW_DRAW_STATE ds;
    e.Dev()->SnapshotDrawState(ds, D3DPT_TRIANGLELIST);
    SWPipelineState sps;
    BuildSWPipelineState(ds, sps);

    ASSERT_EQ(sps.numRenderTargets, 3u);
    EXPECT_NE(sps.renderTargets[0].data, nullptr);   // slot 0 = backbuffer
    EXPECT_EQ(sps.renderTargets[1].data, nullptr);   // slot 1 unbound
    EXPECT_NE(sps.renderTargets[2].data, nullptr);   // slot 2 = rt2

    rt2->Release();
}
