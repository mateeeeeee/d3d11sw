#include <algorithm>
#include <bit>
#include <cstring>
#include <vector>
#include "d3d9/context/pipeline_state_builder.h"
#include "d3d9/common/d3d9_headers.h"
#include "d3d9/resources/cube_texture.h"
#include "d3d9/resources/volume_texture.h"
#include "d3d9/resources/index_buffer.h"
#include "d3d9/resources/surface.h"
#include "d3d9/resources/texture.h"
#include "d3d9/resources/vertex_buffer.h"
#include "d3d9/shaders/pixel_shader.h"
#include "d3d9/shaders/vertex_declaration.h"
#include "d3d9/shaders/vertex_shader.h"
#include "d3d9/translate/fvf.h"
#include "core/pipeline/sw_pipeline_state.h"
#include "core/shaders/shader_abi.h"

namespace d3dsw {

static SWTopology MapTopology(D3DPRIMITIVETYPE pt)
{
    switch (pt)
    {
        case D3DPT_POINTLIST:     return SWTopology::PointList;
        case D3DPT_LINELIST:      return SWTopology::LineList;
        case D3DPT_LINESTRIP:     return SWTopology::LineStrip;
        case D3DPT_TRIANGLELIST:  return SWTopology::TriangleList;
        case D3DPT_TRIANGLESTRIP: return SWTopology::TriangleStrip;
        case D3DPT_TRIANGLEFAN:   return SWTopology::TriangleFan;
        default:                  return SWTopology::Undefined;
    }
}

static DXGI_FORMAT MapDeclType(BYTE type)
{
    switch (type)
    {
        case D3DDECLTYPE_FLOAT1:    return DXGI_FORMAT_R32_FLOAT;
        case D3DDECLTYPE_FLOAT2:    return DXGI_FORMAT_R32G32_FLOAT;
        case D3DDECLTYPE_FLOAT3:    return DXGI_FORMAT_R32G32B32_FLOAT;
        case D3DDECLTYPE_FLOAT4:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case D3DDECLTYPE_D3DCOLOR:  return DXGI_FORMAT_B8G8R8A8_UNORM;
        case D3DDECLTYPE_UBYTE4:    return DXGI_FORMAT_R8G8B8A8_UINT;
        case D3DDECLTYPE_SHORT2:    return DXGI_FORMAT_R16G16_SINT;
        case D3DDECLTYPE_SHORT4:    return DXGI_FORMAT_R16G16B16A16_SINT;
        case D3DDECLTYPE_UBYTE4N:   return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DDECLTYPE_SHORT2N:   return DXGI_FORMAT_R16G16_SNORM;
        case D3DDECLTYPE_SHORT4N:   return DXGI_FORMAT_R16G16B16A16_SNORM;
        case D3DDECLTYPE_USHORT2N:  return DXGI_FORMAT_R16G16_UNORM;
        case D3DDECLTYPE_USHORT4N:  return DXGI_FORMAT_R16G16B16A16_UNORM;
        case D3DDECLTYPE_FLOAT16_2: return DXGI_FORMAT_R16G16_FLOAT;
        case D3DDECLTYPE_FLOAT16_4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        //no direct DXGI equivalent.
        case D3DDECLTYPE_UDEC3:
        case D3DDECLTYPE_DEC3N:
        default:                    return DXGI_FORMAT_UNKNOWN;
    }
}

static const Char* MapDeclUsage(BYTE usage)
{
    switch (usage)
    {
        case D3DDECLUSAGE_POSITION:     return "POSITION";
        case D3DDECLUSAGE_BLENDWEIGHT:  return "BLENDWEIGHT";
        case D3DDECLUSAGE_BLENDINDICES: return "BLENDINDICES";
        case D3DDECLUSAGE_NORMAL:       return "NORMAL";
        case D3DDECLUSAGE_PSIZE:        return "PSIZE";
        case D3DDECLUSAGE_TEXCOORD:     return "TEXCOORD";
        case D3DDECLUSAGE_TANGENT:      return "TANGENT";
        case D3DDECLUSAGE_BINORMAL:     return "BINORMAL";
        case D3DDECLUSAGE_TESSFACTOR:   return "TESSFACTOR";
        case D3DDECLUSAGE_POSITIONT:    return "POSITIONT";
        case D3DDECLUSAGE_COLOR:        return "COLOR";
        case D3DDECLUSAGE_FOG:          return "FOG";
        case D3DDECLUSAGE_DEPTH:        return "DEPTH";
        case D3DDECLUSAGE_SAMPLE:       return "SAMPLE";
        default:                        return "";
    }
}

static void WriteInputElements(const D3DVERTEXELEMENT9* decl, UINT count, const UINT* streamFreq, SWPipelineState& out)
{
    count = std::min(count, static_cast<Uint>(SW_MAX_INPUT_ELEMENTS));
    for (Uint i = 0; i < count; ++i)
    {
        const D3DVERTEXELEMENT9& e = decl[i];
        SWInputElement& dst = out.inputElements[i];
        const Char* sem = MapDeclUsage(e.Usage);
        std::strncpy(dst.semanticName, sem, SW_SEMANTIC_NAME_MAX - 1);
        dst.semanticName[SW_SEMANTIC_NAME_MAX - 1] = '\0';
        dst.semanticIndex        = e.UsageIndex;
        dst.format               = MapDeclType(e.Type);
        dst.inputSlot            = e.Stream;
        dst.alignedByteOffset    = e.Offset;
        Uint freq = streamFreq ? streamFreq[e.Stream] : 0u;
        if (freq & D3DSTREAMSOURCE_INSTANCEDATA)
        {
            dst.inputSlotClass       = SWInputSlotClass::PerInstanceData;
            dst.instanceDataStepRate = std::max(1u, freq & 0x3FFFFFFFu);
        }
        else
        {
            dst.inputSlotClass       = SWInputSlotClass::PerVertexData;
            dst.instanceDataStepRate = 0;
        }
    }
    out.numInputElements = count;
}

static void FillInputElements(const D3D9SW_DRAW_STATE& in, SWPipelineState& out)
{
    out.numInputElements = 0;

    //VertexDeclaration takes precedence, FVF is the fallback for fixed-function legacy draws that never called SetVertexDeclaration.
    if (in.vertexDecl)
    {
        WriteInputElements(in.vertexDecl->Elements(), in.vertexDecl->Count(), in.streamFreq, out);
        return;
    }

    if (in.fvf != 0)
    {
        std::vector<D3DVERTEXELEMENT9> synthesized;
        FVFToDeclaration(in.fvf, synthesized);
        //synthesized array ends with D3DDECL_END, drop it
        Uint count = synthesized.empty() ? 0u : static_cast<Uint>(synthesized.size() - 1);
        WriteInputElements(synthesized.data(), count, in.streamFreq, out);
    }
}

static void FillVertexBuffers(const D3D9SW_DRAW_STATE& in, SWPipelineState& out)
{
    Uint n = std::min<Uint>(SW_D3D9_MAX_STREAMS, SW_MAX_VERTEX_BUFFERS);
    for (Uint i = 0; i < n; ++i)
    {
        SWVertexBufferBinding& dst = out.vertexBuffers[i];
        const auto& s = in.streams[i];
        if (s.buffer)
        {
            dst.data   = s.buffer->DataPtr() + s.offset;
            dst.stride = s.stride;
            dst.offset = 0;   
        }
        else
        {
            dst.data   = nullptr;
            dst.stride = 0;
            dst.offset = 0;
        }
    }
}

static void FillIndexBuffer(const D3D9SW_DRAW_STATE& in, SWPipelineState& out)
{
    out.indexBuffer = {};
    if (!in.indices)
    {
        return;
    }
    out.indexBuffer.data = in.indices->DataPtr();
    out.indexBuffer.format = (in.indices->Format() == D3DFMT_INDEX32) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    out.indexBuffer.offset = 0;
}

static void FillRenderTargets(const D3D9SW_DRAW_STATE& in, SWPipelineState& out)
{
    out.numRenderTargets = 0;
    Uint n = std::min<Uint>(in.numRenderTargets, SW_MAX_RENDER_TARGETS);
    for (Uint i = 0; i < n; ++i)
    {
        D3D9SurfaceSW* surf = in.renderTargets[i];
        SWRenderTargetSnapshot& dst = out.renderTargets[i];
        dst = {};
        if (!surf)
        {
            continue;
        }
        dst.data          = surf->DataPtr();
        dst.format        = surf->DxgiFormat();
        dst.rowPitch      = surf->RowPitch();
        dst.pixStride     = surf->PixStride();
        dst.slicePitch    = surf->RowPitch() * surf->Height();
        dst.sampleCount   = 1;
        dst.sampleQuality = 0;
        dst.arraySize     = 1;
    }
    out.numRenderTargets = n;
}

static void FillDepthStencil(const D3D9SW_DRAW_STATE& in, SWPipelineState& out)
{
    out.depthStencilView = {};
    if (!in.depthStencil)
    {
        return;
    }
    D3D9SurfaceSW* dsv = in.depthStencil;
    out.depthStencilView.data          = dsv->DataPtr();
    out.depthStencilView.format        = dsv->DxgiFormat();
    out.depthStencilView.rowPitch      = dsv->RowPitch();
    out.depthStencilView.numRows       = dsv->Height();
    out.depthStencilView.pixStride     = dsv->PixStride();
    out.depthStencilView.sampleCount   = 1;
    out.depthStencilView.sampleQuality = 0;
}

static void FillViewport(const D3D9SW_DRAW_STATE& in, SWPipelineState& out)
{
    out.numViewports = 1;
    out.viewports[0] = {
        static_cast<Float>(in.viewport.X),
        static_cast<Float>(in.viewport.Y),
        static_cast<Float>(in.viewport.Width),
        static_cast<Float>(in.viewport.Height),
        in.viewport.MinZ,
        in.viewport.MaxZ,
    };
    out.numScissorRects = 1;
    out.scissorRects[0] = {
        static_cast<Int32>(in.scissor.left),
        static_cast<Int32>(in.scissor.top),
        static_cast<Int32>(in.scissor.right),
        static_cast<Int32>(in.scissor.bottom),
    };
}

static void FillShaders(const D3D9SW_DRAW_STATE& in, SWPipelineState& out)
{
    if (in.vs)
    {
        out.vs.fn         = reinterpret_cast<void*>(in.vs->GetJitFn());
        out.vs.reflection = &in.vs->GetReflection();
        out.vsRes.cb[0]   = reinterpret_cast<const SW_float4*>(in.vsConstF);
    }
    else if (in.ffVsFn)
    {
        out.vs.fn         = in.ffVsFn;
        out.vs.reflection = in.ffVsRefl;
        out.vsRes.cb[0]   = reinterpret_cast<const SW_float4*>(in.vsConstF);
    }

    if (in.ps)
    {
        out.ps.fn         = reinterpret_cast<void*>(in.ps->GetJitFn());
        out.ps.reflection = &in.ps->GetReflection();
        out.psRes.cb[0]   = reinterpret_cast<const SW_float4*>(in.psConstF);
    }
    else if (in.ffPsFn)
    {
        out.ps.fn         = in.ffPsFn;
        out.ps.reflection = in.ffPsRefl;
    }
}

// Map D3D9 min/mag/mip filter triad to a D3D11-style filter encoding.
static SWFilter MapD3D9Filter(DWORD minF, DWORD magF, DWORD mipF)
{
    if (minF == D3DTEXF_ANISOTROPIC || magF == D3DTEXF_ANISOTROPIC) { return 0x55u; }
    Uint32 f = 0;
    if (minF == D3DTEXF_LINEAR) { f |= 0x10u; }
    if (magF == D3DTEXF_LINEAR) { f |= 0x04u; }
    if (mipF == D3DTEXF_LINEAR) { f |= 0x01u; }
    return f;
}

static void FillTextures(const D3D9SW_DRAW_STATE& in, SWPipelineState& out)
{
    for (Uint s = 0; s < D3D9SW_DRAW_STATE::SW_D3D9_MAX_TEX; ++s)
    {
        auto* baseTexture = in.textures[s];
        if (!baseTexture) 
        { 
            continue; 
        }

        {
            IDirect3DTexture9* tex9 = nullptr;
            if (SUCCEEDED(baseTexture->QueryInterface(IID_IDirect3DTexture9,
                                                      reinterpret_cast<void**>(&tex9))))
            {
                D3D9TextureSW* tex = static_cast<D3D9TextureSW*>(tex9);
                tex->FillSRV(out.psRes.srv[s]);
                tex9->Release();
                goto sampler;
            }
        }
        {
            IDirect3DCubeTexture9* cube9 = nullptr;
            if (SUCCEEDED(baseTexture->QueryInterface(IID_IDirect3DCubeTexture9,
                                                      reinterpret_cast<void**>(&cube9))))
            {
                D3D9CubeTextureSW* cube = static_cast<D3D9CubeTextureSW*>(cube9);
                cube->FillSRV(out.psRes.srv[s]);
                cube9->Release();
                goto sampler;
            }
        }
        {
            IDirect3DVolumeTexture9* vol9 = nullptr;
            if (SUCCEEDED(baseTexture->QueryInterface(IID_IDirect3DVolumeTexture9,
                                                      reinterpret_cast<void**>(&vol9))))
            {
                D3D9VolumeTextureSW* vol = static_cast<D3D9VolumeTextureSW*>(vol9);
                vol->FillSRV(out.psRes.srv[s]);
                vol9->Release();
                goto sampler;
            }
        }
        continue;

    sampler:
        const DWORD* ss = in.samplerStates[s];
        SW_Sampler& smp  = out.psRes.smp[s];
        smp.addressU        = ss[D3DSAMP_ADDRESSU];
        smp.addressV        = ss[D3DSAMP_ADDRESSV];
        smp.addressW        = ss[D3DSAMP_ADDRESSW];
        smp.filter          = MapD3D9Filter(ss[D3DSAMP_MINFILTER],
                                            ss[D3DSAMP_MAGFILTER],
                                            ss[D3DSAMP_MIPFILTER]);
        smp.mipLODBias      = std::bit_cast<float>(ss[D3DSAMP_MIPMAPLODBIAS]);
        smp.minLOD          = 0.f;
        smp.maxLOD          = (ss[D3DSAMP_MIPFILTER] == D3DTEXF_NONE) ? 0.f : 1000.f;
        smp.maxAnisotropy   = ss[D3DSAMP_MAXANISOTROPY];
        smp.comparisonFunc  = 0;
    }
}

void BuildSWPipelineState(const D3D9SW_DRAW_STATE& in, SWPipelineState& out)
{
    out = {};
    out.topology = MapTopology(in.primitiveType);

    FillInputElements(in, out);
    FillVertexBuffers(in, out);
    FillIndexBuffer(in, out);
    FillRenderTargets(in, out);
    FillDepthStencil(in, out);
    FillViewport(in, out);
    FillShaders(in, out);
    FillTextures(in, out);

    //D3DCULL requires a frontCCW derivation (D3D9 names the face to CULL;
    //SW names which face is FRONT). D3DCULL_CCW culls CCW-in-screen = keeps CW
    //= front is CW-in-screen = area < 0 → frontCCW=true keeps those
    const DWORD* rs = in.rs;
    {
        auto& rd = out.rasterizerDesc;
        switch (rs[D3DRS_CULLMODE])
        {
            case D3DCULL_NONE: rd.cullMode = SWCullMode::None;  rd.frontCCW = false; break;
            case D3DCULL_CW:   rd.cullMode = SWCullMode::Back;  rd.frontCCW = true;  break; 
            default:           rd.cullMode = SWCullMode::Back;  rd.frontCCW = false; break; 
        }
        switch (rs[D3DRS_FILLMODE])
        {
            case D3DFILL_WIREFRAME: rd.fillMode = SWFillMode::Wireframe; break;
            default:                rd.fillMode = SWFillMode::Solid;     break;
        }
        rd.depthBias             = 0;
        rd.depthBiasClamp        = 0.f;
        rd.slopeScaledDepthBias  = std::bit_cast<Float>(rs[D3DRS_SLOPESCALEDEPTHBIAS]);
        rd.depthClipEnable       = true;
        rd.scissorEnable         = rs[D3DRS_SCISSORTESTENABLE] ? true : false;
        rd.multisampleEnable     = false;
        rd.antialiasedLineEnable = false;
    }

    {
        auto& dsd = out.depthStencilDesc;
        dsd.depthEnable      = (in.depthStencil != nullptr) && (rs[D3DRS_ZENABLE] != D3DZB_FALSE);
        dsd.depthWriteMask   = rs[D3DRS_ZWRITEENABLE] ? SWDepthWriteMask::All : SWDepthWriteMask::Zero;
        dsd.depthFunc        = static_cast<SWComparison>(rs[D3DRS_ZFUNC]);
        dsd.stencilEnable    = rs[D3DRS_STENCILENABLE] ? true : false;
        dsd.stencilReadMask  = static_cast<Uint8>(rs[D3DRS_STENCILMASK]      & 0xFF);
        dsd.stencilWriteMask = static_cast<Uint8>(rs[D3DRS_STENCILWRITEMASK] & 0xFF);

        auto mapOp = [](DWORD v) { return static_cast<SWStencilOp>(v); };
        dsd.frontFace = 
        {
            mapOp(rs[D3DRS_STENCILFAIL]),
            mapOp(rs[D3DRS_STENCILZFAIL]),
            mapOp(rs[D3DRS_STENCILPASS]),
            static_cast<SWComparison>(rs[D3DRS_STENCILFUNC]),
        };
        if (rs[D3DRS_TWOSIDEDSTENCILMODE])
        {
            dsd.backFace = 
            {
                mapOp(rs[D3DRS_CCW_STENCILFAIL]),
                mapOp(rs[D3DRS_CCW_STENCILZFAIL]),
                mapOp(rs[D3DRS_CCW_STENCILPASS]),
                static_cast<SWComparison>(rs[D3DRS_CCW_STENCILFUNC]),
            };
        }
        else
        {
            dsd.backFace = dsd.frontFace;
        }
    }

    {
        static constexpr DWORD cwRegs[4] = 
        {
            D3DRS_COLORWRITEENABLE,  D3DRS_COLORWRITEENABLE1,
            D3DRS_COLORWRITEENABLE2, D3DRS_COLORWRITEENABLE3,
        };
        auto& bd = out.blendDesc;
        bd.alphaToCoverage  = false;
        bd.independentBlend = false;
        for (Uint i = 0; i < 8; ++i)
        {
            auto& rt          = bd.renderTarget[i];
            rt.blendEnable    = rs[D3DRS_ALPHABLENDENABLE] ? true : false;
            rt.logicOpEnable  = false;
            rt.srcBlend       = static_cast<SWBlend>(rs[D3DRS_SRCBLEND]);
            rt.destBlend      = static_cast<SWBlend>(rs[D3DRS_DESTBLEND]);
            rt.blendOp        = static_cast<SWBlendOp>(rs[D3DRS_BLENDOP]);
            rt.srcBlendAlpha  = static_cast<SWBlend>(rs[D3DRS_SRCBLENDALPHA]);
            rt.destBlendAlpha = static_cast<SWBlend>(rs[D3DRS_DESTBLENDALPHA]);
            rt.blendOpAlpha   = static_cast<SWBlendOp>(rs[D3DRS_BLENDOPALPHA]);
            rt.logicOp        = SWLogicOp::Noop;
            DWORD cwIdx = (i < 4) ? cwRegs[i] : cwRegs[3];
            rt.renderTargetWriteMask = static_cast<Uint8>(rs[cwIdx] & 0xF);
        }
    }

    out.alphaTestEnable = rs[D3DRS_ALPHATESTENABLE] ? true : false;
    out.alphaTestFunc   = static_cast<SWComparison>(rs[D3DRS_ALPHAFUNC]);
    out.alphaRef        = static_cast<Uint8>(rs[D3DRS_ALPHAREF] & 0xFF);

    {
        auto& fog = out.fogDesc;
        fog.enable = rs[D3DRS_FOGENABLE] ? true : false;
        if (fog.enable)
        {
            DWORD c       = rs[D3DRS_FOGCOLOR];
            fog.color[0]  = ((c >> 16) & 0xFF) / 255.f;
            fog.color[1]  = ((c >>  8) & 0xFF) / 255.f;
            fog.color[2]  = ((c >>  0) & 0xFF) / 255.f;
            fog.color[3]  = ((c >> 24) & 0xFF) / 255.f;
            fog.start     = std::bit_cast<Float>(rs[D3DRS_FOGSTART]);
            fog.end       = std::bit_cast<Float>(rs[D3DRS_FOGEND]);
            fog.density   = std::bit_cast<Float>(rs[D3DRS_FOGDENSITY]);
            auto mapMode  = [](DWORD m) -> SWPipelineState::SWFogMode {
                switch (m) 
                {
                    case D3DFOG_LINEAR: return SWPipelineState::SWFogMode::Linear;
                    case D3DFOG_EXP:    return SWPipelineState::SWFogMode::Exp;
                    case D3DFOG_EXP2:   return SWPipelineState::SWFogMode::Exp2;
                    default:            return SWPipelineState::SWFogMode::None;
                }
            };
            fog.vertexMode = mapMode(rs[D3DRS_FOGVERTEXMODE]);
            fog.tableMode  = mapMode(rs[D3DRS_FOGTABLEMODE]);
        }
    }

    out.sampleMask     = 0xFFFFFFFFu;
    out.blendFactor[0] = out.blendFactor[1] = out.blendFactor[2] = out.blendFactor[3] = 0.f;
    out.stencilRef     = static_cast<Uint8>(rs[D3DRS_STENCILREF] & 0xFF);
    out.stats          = in.stats;
}

}
