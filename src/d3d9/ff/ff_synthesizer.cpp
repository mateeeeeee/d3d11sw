#include <bit>
#include <cstring>
#include <d3d11TokenizedProgramFormat.hpp>
#include <d3dcommon.h>
#include "d3d9/ff/ff_synthesizer.h"
#include "core/shaders/sm4.h"

namespace d3dsw {

//CB slot 0 layout when lighting is enabled:
//  [0..3]      WVP columns (world * view * projection)
//  [4..7]      WorldView columns (world * view)  — eye-space position
//  [8..10]     WorldView upper-left 3×3 rows      — eye-space normal transform
//  [11]        (padding)
//  [12]        global ambient (D3DRS_AMBIENT as float4)
//  [13]        material.ambient
//  [14]        material.diffuse
//  [15]        material.specular
//  [16]        material.emissive
//  [17]        (material.power, 0, 0, 0)
//  Per enabled light, 8 rows each at CB_LIGHT_BASE + i*CB_LIGHT_ROWS:
//    +0  (lightPos_eye.xyz, 0)   for point/spot; (0,0,0,0) for dir
//    +1  L direction in eye space (normalize(-dir) for dir; normalize(dir) for spot)
//    +2  light.ambient
//    +3  light.diffuse
//    +4  light.specular
//    +5  (att0, att1, att2, range)
//    +6  (cos(theta/2), cos(phi/2), falloff, 0)   spot params
//    +7  (padding)
static constexpr Uint32 CB_WVP_COL0   = 0;
static constexpr Uint32 CB_WV_COL0    = 4;
static constexpr Uint32 CB_WV_IT_ROW0 = 8;
static constexpr Uint32 CB_GLOBAL_AMB = 12;
static constexpr Uint32 CB_MAT_AMB    = 13;
static constexpr Uint32 CB_MAT_DIFF   = 14;
static constexpr Uint32 CB_MAT_SPEC   = 15;
static constexpr Uint32 CB_MAT_EMIS   = 16;
static constexpr Uint32 CB_MAT_POWER  = 17;
static constexpr Uint32 CB_LIGHT_BASE = 18;
static constexpr Uint32 CB_LIGHT_ROWS = 8;
static constexpr Uint32 CB_FOG_PARAMS = 100;  //.x=start .y=end .z=density_packed .w=1/(end-start)
static constexpr Uint32 CB_TEX_XFORM_BASE = 104; // 4 CB rows per stage × 8 stages = rows 104-135
//Per-light row offsets:
static constexpr Uint32 LR_POSITION   = 0;
static constexpr Uint32 LR_DIRECTION  = 1;
static constexpr Uint32 LR_AMBIENT    = 2;
static constexpr Uint32 LR_DIFFUSE    = 3;
static constexpr Uint32 LR_SPECULAR   = 4;
static constexpr Uint32 LR_ATT        = 5;  //(att0, att1, att2, range)
static constexpr Uint32 LR_SPOT       = 6;  //(cos(theta/2), cos(phi/2), falloff, 0)

static SM4Operand InputOp(Uint32 reg)
{
    SM4Operand op{};
    op.type = D3D10_SB_OPERAND_TYPE_INPUT;
    op.indexDim = 1;
    op.indices[0] = reg;
    op.swizzle[0] = 0; op.swizzle[1] = 1; op.swizzle[2] = 2; op.swizzle[3] = 3;
    return op;
}
static SM4Operand OutputOp(Uint32 reg, Uint8 mask = 0xF)
{
    SM4Operand op{};
    op.type = D3D10_SB_OPERAND_TYPE_OUTPUT;
    op.indexDim = 1;
    op.indices[0] = reg;
    op.writeMask = mask;
    return op;
}
static SM4Operand TempOp(Uint32 reg, Uint8 mask = 0xF)
{
    SM4Operand op{};
    op.type = D3D10_SB_OPERAND_TYPE_TEMP;
    op.indexDim = 1;
    op.indices[0] = reg;
    op.writeMask = mask;
    op.swizzle[0] = 0; op.swizzle[1] = 1; op.swizzle[2] = 2; op.swizzle[3] = 3;
    return op;
}
static SM4Operand CBOp(Uint32 slot, Uint32 idx)
{
    SM4Operand op{};
    op.type = D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER;
    op.indexDim = 2;
    op.indices[0] = slot;
    op.indices[1] = idx;
    op.swizzle[0] = 0; op.swizzle[1] = 1; op.swizzle[2] = 2; op.swizzle[3] = 3;
    return op;
}
static SM4Operand ResourceOp(Uint32 slot)
{
    SM4Operand op{};
    op.type = D3D10_SB_OPERAND_TYPE_RESOURCE;
    op.indexDim = 1;
    op.indices[0] = slot;
    op.swizzle[0] = 0; op.swizzle[1] = 1; op.swizzle[2] = 2; op.swizzle[3] = 3;
    return op;
}
static SM4Operand SamplerOp(Uint32 slot)
{
    SM4Operand op{};
    op.type = D3D10_SB_OPERAND_TYPE_SAMPLER;
    op.indexDim = 1;
    op.indices[0] = slot;
    return op;
}
static SM4Operand ImmF(Float x, Float y, Float z, Float w)
{
    SM4Operand op{};
    op.type = D3D10_SB_OPERAND_TYPE_IMMEDIATE32;
    op.indexDim = 0;
    op.imm[0] = x; op.imm[1] = y; op.imm[2] = z; op.imm[3] = w;
    op.swizzle[0] = 0; op.swizzle[1] = 1; op.swizzle[2] = 2; op.swizzle[3] = 3;
    return op;
}

static SM4Operand Swz(SM4Operand op, Uint8 c)
{
    op.swizzle[0] = op.swizzle[1] = op.swizzle[2] = op.swizzle[3] = c;
    return op;
}
static SM4Operand Neg(SM4Operand op) { op.negate = true; return op; }

static D3DSW_ShaderSignatureElement MakeSig(const Char* name, Uint32 semIdx,
                                             Uint32 reg, Uint8 mask, Uint32 svType)
{
    D3DSW_ShaderSignatureElement e{};
    std::strncpy(e.name, name, sizeof(e.name) - 1);
    e.semanticIndex = semIdx;
    e.reg    = reg;
    e.mask   = mask;
    e.svType = svType;
    return e;
}
static SM4Instruction Instr(D3D10_SB_OPCODE_TYPE op) { SM4Instruction i{}; i.op = op; return i; }
static SM4Instruction InstrSat(D3D10_SB_OPCODE_TYPE op) { SM4Instruction i{}; i.op = op; i.saturate = true; return i; }

//Emit: DP4 dst.{comp}, src0, cb[0][row]
static void EmitDP4Col(D3DSW_ParsedShader& out, SM4Operand dst, SM4Operand src,
                       Uint32 cbRow)
{
    auto dp4 = Instr(D3D10_SB_OPCODE_DP4);
    dp4.operands = { dst, src, CBOp(0, cbRow) };
    out.instrs.push_back(dp4);
}
//Emit: DP3 dst.{comp}, src0, cb[0][row]
static void EmitDP3Row(D3DSW_ParsedShader& out, SM4Operand dst, SM4Operand src,
                       Uint32 cbRow)
{
    auto dp3 = Instr(D3D10_SB_OPCODE_DP3);
    dp3.operands = { dst, src, CBOp(0, cbRow) };
    out.instrs.push_back(dp3);
}

//Resolve a D3D9 material colour source to the correct SM4 operand.
//src: 0=MATERIAL (CB), 1=COLOR1 (vertex diffuse), 2=COLOR2 (vertex specular).
//Falls back to CB when the vertex register is unavailable.
static SM4Operand ResolveMatSrc(Uint8 colorVertex, Uint8 src,
                                 Uint32 diffIn, Uint32 specIn, Uint32 cbRow)
{
    if (colorVertex)
    {
        if (src == 1 && diffIn != 0xFFu) { return InputOp(diffIn); }
        if (src == 2 && specIn != 0xFFu) { return InputOp(specIn); }
    }
    return CBOp(0, cbRow);
}


// ---------------------------------------------------------------------------
void SynthesizeFFVS(const FFVSKey& key, D3DSW_ParsedShader& out)
{
    out = {};
    out.type = D3DSW_ShaderType::Vertex;

    const Bool hasDiffuseFVF  = (key.fvf & D3DFVF_DIFFUSE)  != 0;
    const Bool hasSpecularFVF = (key.fvf & D3DFVF_SPECULAR) != 0;
    const Bool hasNormal      = (key.fvf & D3DFVF_NORMAL)    != 0;
    const Uint numTex         = key.numTexCoords;
    const Bool preTransformed = (key.fvf & D3DFVF_XYZRHW)    != 0;
    const Bool lightingOn     = key.lightingEnabled != 0;
    const Bool specularOn     = key.specularEnabled  != 0;
    const Uint8 cv            = key.colorVertex;

    Uint32 nextIn   = 0;
    Uint32 posIn    = nextIn++;                       // v[0] = POSITION (always)
    Uint32 normIn   = 0xFFu;
    Uint32 diffIn   = 0xFFu;
    Uint32 specIn   = 0xFFu;
    Uint32 tcIn[8]; 
    for (UINT t = 0; t < 8; ++t) tcIn[t] = 0xFFu;

    out.inputs.push_back(MakeSig("POSITION", 0, posIn, 0xF, 0));
    if (hasNormal && !preTransformed)
    {
        normIn = nextIn;
        out.inputs.push_back(MakeSig("NORMAL", 0, nextIn++, 0x7, 0));
    }

    //Vertex diffuse needed when:
    //  (a) no lighting —> passthrough to output
    //  (b) lighting + colorVertex + any material source uses COLOR1 (diffuse)
    const Bool needDiffIn = hasDiffuseFVF && (
        !lightingOn ||
        (lightingOn && cv && (key.diffuseSrc == 1 || key.ambientSrc == 1 || key.emissiveSrc == 1)));
    if (needDiffIn)
    {
        diffIn = nextIn;
        out.inputs.push_back(MakeSig("COLOR", 0, nextIn++, 0xF, 0));
    }

    // Vertex specular needed when lighting + colorVertex + any material source uses COLOR2 (specular)
    const Bool needSpecIn = hasSpecularFVF && lightingOn && cv &&
                            (key.diffuseSrc == 2 || key.specularSrc == 2 ||
                             key.ambientSrc == 2 || key.emissiveSrc == 2);
    if (needSpecIn)
    {
        specIn = nextIn;
        out.inputs.push_back(MakeSig("COLOR", 1, nextIn++, 0xF, 0));
    }
    for (Uint t = 0; t < numTex && t < 8; ++t)
    {
        tcIn[t] = nextIn;
        out.inputs.push_back(MakeSig("TEXCOORD", t, nextIn++, 0x3, 0));
    }

    Uint32 nextOut  = 0;
    Uint32 posOut   = nextOut++;
    Uint32 colorOut = 0xFFu;
    Uint32 specOut  = 0xFFu;
    Uint32 tcOut[8]; 
    for (Uint t = 0; t < 8; ++t) tcOut[t] = 0xFFu;

    out.outputs.push_back(MakeSig("SV_Position", 0, posOut, 0xF, D3D_NAME_POSITION));
    //COLOR0: present when FVF has diffuse or lighting is on
    if (hasDiffuseFVF || lightingOn)
    {
        colorOut = nextOut;
        out.outputs.push_back(MakeSig("COLOR", 0, nextOut++, 0xF, 0));
    }
    //COLOR1: specular output when lighting+specular enabled
    if (lightingOn && specularOn)
    {
        specOut = nextOut;
        out.outputs.push_back(MakeSig("COLOR", 1, nextOut++, 0xF, 0));
    }
    for (Uint t = 0; t < numTex && t < 8; ++t)
    {
        tcOut[t] = nextOut;
        out.outputs.push_back(MakeSig("TEXCOORD", t, nextOut++, 0x3, 0));
    }

    Uint32 fogOut = 0xFFu;
    if (!preTransformed && key.fogVertexMode != 0)
    {
        fogOut = nextOut;
        out.outputs.push_back(MakeSig("FOG", 0, nextOut++, 0x1, 0));
    }

    {
        D3DSW_CBufBinding cbuf{}; cbuf.slot = 0;
        if (preTransformed)
        {
            cbuf.sizeVec4s = 0;
        }
        else if (lightingOn)
        {
            Uint32 maxLightSlot = 0;
            for (Uint32 i = 0; i < 8; ++i)
            {
                if (key.lightMask & (1u << i)) 
                { 
                    maxLightSlot = i + 1; 
                }
            }
            cbuf.sizeVec4s = CB_LIGHT_BASE + maxLightSlot * CB_LIGHT_ROWS;
        }
        else
        {
            cbuf.sizeVec4s = (key.fogVertexMode != 0) ? 8u : 4u;
        }
        if (key.fogVertexMode != 0)
        {
            cbuf.sizeVec4s = std::max(cbuf.sizeVec4s, CB_FOG_PARAMS + 1u);
        }

        for (Uint t = 0; t < 8; ++t)
        {
            if (key.texTransformFlags[t] & 0xFF)
            {
                cbuf.sizeVec4s = std::max(cbuf.sizeVec4s, CB_TEX_XFORM_BASE + (t + 1) * 4u);
            }
        }
        if (cbuf.sizeVec4s > 0) 
        { 
            out.cbufs.push_back(cbuf); 
        }
    }

    //Temp register plan (for lit path):
    //  r0 = eye-space position (xyzw)
    //  r1 = eye-space normal (xyz, normalized)
    //  r2 = diffuse accumulator (rgba)
    //  r3 = specular accumulator (rgba)
    //  r4 = per-light L vector (xyz)
    //  r5 = per-light attenuation scalar in .x, d in .y
    //  r6 = per-light NdotL in .x, NdotH / misc in .y
    //  r7 = scratch
    out.numTemps = lightingOn ? 8u : 0u;
    if (!preTransformed && key.fogVertexMode != 0)
    {
        out.numTemps = std::max(out.numTemps, lightingOn ? 8u : 1u);
    }

    // Texture transforms need a scratch temp (r8 for lit; reuse r0 for non-lit if fog not active,
    // otherwise r1). Use r8 uniformly to avoid conflicts.
    Bool anyTexTransform = false;
    for (Uint t = 0; t < 8; ++t) 
    { 
        if (key.texTransformFlags[t] & 0xFF) 
        { 
            anyTexTransform = true; 
            break; 
        } 
    }
    const Uint32 tcXformScratch = 8u;   
    if (!preTransformed && anyTexTransform)
    {
        out.numTemps = std::max(out.numTemps, tcXformScratch + 1u);
    }

    if (preTransformed)
    {
        auto mv = Instr(D3D10_SB_OPCODE_MOV);
        mv.operands = { OutputOp(posOut), InputOp(posIn) };
        out.instrs.push_back(mv);
    }
    else
    {
        // WVP transform → clip position
        for (Uint32 col = 0; col < 4; ++col)
        {
            EmitDP4Col(out, OutputOp(posOut, 1u << col), InputOp(posIn), CB_WVP_COL0 + col);
        }
    }

    if (lightingOn)
    {
        // Eye-space position (for point/spot lights)
        for (Uint32 col = 0; col < 4; ++col)
        {
            EmitDP4Col(out, TempOp(0, 1u << col), InputOp(posIn), CB_WV_COL0 + col);
        }

        // Eye-space normal via WorldView upper-left 3×3 (row-major in CB)
        EmitDP3Row(out, TempOp(1, 0x1), InputOp(normIn), CB_WV_IT_ROW0 + 0);
        EmitDP3Row(out, TempOp(1, 0x2), InputOp(normIn), CB_WV_IT_ROW0 + 1);
        EmitDP3Row(out, TempOp(1, 0x4), InputOp(normIn), CB_WV_IT_ROW0 + 2);

        // Normalize normal: r7.x = 1/|r1|, r1 *= r7.x
        {
            auto dp3 = Instr(D3D10_SB_OPCODE_DP3);
            dp3.operands = { TempOp(7, 0x1), TempOp(1), TempOp(1) };
            out.instrs.push_back(dp3);

            auto rsq = Instr(D3D10_SB_OPCODE_RSQ);
            rsq.operands = { TempOp(7, 0x1), Swz(TempOp(7), 0) };
            out.instrs.push_back(rsq);

            auto mul = Instr(D3D10_SB_OPCODE_MUL);
            mul.operands = { TempOp(1, 0x7), TempOp(1), Swz(TempOp(7), 0) };
            out.instrs.push_back(mul);
        }

        // r2 = emissive + globalAmbient * matAmbient
        {
            SM4Operand matEmis = ResolveMatSrc(cv, key.emissiveSrc, diffIn, specIn, CB_MAT_EMIS);
            SM4Operand matAmb  = ResolveMatSrc(cv, key.ambientSrc,  diffIn, specIn, CB_MAT_AMB);

            auto mv = Instr(D3D10_SB_OPCODE_MOV);
            mv.operands = { TempOp(2), matEmis };
            out.instrs.push_back(mv);

            auto mul = Instr(D3D10_SB_OPCODE_MUL);
            mul.operands = { TempOp(7), CBOp(0, CB_GLOBAL_AMB), matAmb };
            out.instrs.push_back(mul);

            auto add = Instr(D3D10_SB_OPCODE_ADD);
            add.operands = { TempOp(2), TempOp(2), TempOp(7) };
            out.instrs.push_back(add);
        }

        if (specularOn)
        {
            auto mv = Instr(D3D10_SB_OPCODE_MOV);
            mv.operands = { TempOp(3), ImmF(0, 0, 0, 0) };
            out.instrs.push_back(mv);
        }

        for (Uint32 i = 0; i < 8; ++i)
        {
            if (!(key.lightMask & (1u << i))) 
            { 
                continue; 
            }

            const Uint32 lb = CB_LIGHT_BASE + i * CB_LIGHT_ROWS;
            const auto lt = static_cast<D3DLIGHTTYPE>(key.lightTypes[i]);
            if (lt == D3DLIGHT_DIRECTIONAL)
            {
                // L = cb[0][lb+LR_DIRECTION]  (pre-computed as normalize(-dir) eye space)
                {
                    auto mv = Instr(D3D10_SB_OPCODE_MOV);
                    mv.operands = { TempOp(4, 0x7), CBOp(0, lb + LR_DIRECTION) };
                    out.instrs.push_back(mv);
                }
                // att = 1
                {
                    auto mv = Instr(D3D10_SB_OPCODE_MOV);
                    mv.operands = { TempOp(5, 0x1), ImmF(1, 0, 0, 0) };
                    out.instrs.push_back(mv);
                }
            }
            else
            {
                // r4.xyz = lightPos_eye - eyePos
                {
                    auto add = Instr(D3D10_SB_OPCODE_ADD);
                    add.operands = { TempOp(4, 0x7), CBOp(0, lb + LR_POSITION), Neg(TempOp(0)) };
                    out.instrs.push_back(add);
                }
                // r5.x = d^2 = dot3(r4, r4)
                {
                    auto dp3 = Instr(D3D10_SB_OPCODE_DP3);
                    dp3.operands = { TempOp(5, 0x1), TempOp(4), TempOp(4) };
                    out.instrs.push_back(dp3);
                }
                // r6.x = 1/d = rsq(d^2)
                {
                    auto rsq = Instr(D3D10_SB_OPCODE_RSQ);
                    rsq.operands = { TempOp(6, 0x1), Swz(TempOp(5), 0) };
                    out.instrs.push_back(rsq);
                }
                // r5.y = d = d^2 * (1/d)
                {
                    auto mul = Instr(D3D10_SB_OPCODE_MUL);
                    mul.operands = { TempOp(5, 0x2), Swz(TempOp(5), 0), Swz(TempOp(6), 0) };
                    out.instrs.push_back(mul);
                }
                // Normalize r4: r4.xyz = r4 * (1/d)
                {
                    auto mul = Instr(D3D10_SB_OPCODE_MUL);
                    mul.operands = { TempOp(4, 0x7), TempOp(4), Swz(TempOp(6), 0) };
                    out.instrs.push_back(mul);
                }
                // Attenuation: r7.x = att1 * d
                {
                    auto mul = Instr(D3D10_SB_OPCODE_MUL);
                    mul.operands = { TempOp(7, 0x1), Swz(CBOp(0, lb + LR_ATT), 1), Swz(TempOp(5), 1) };
                    out.instrs.push_back(mul);
                }
                // r7.x += att2 * d^2
                {
                    auto mad = Instr(D3D10_SB_OPCODE_MAD);
                    mad.operands = { TempOp(7, 0x1), Swz(CBOp(0, lb + LR_ATT), 2), Swz(TempOp(5), 0), Swz(TempOp(7), 0) };
                    out.instrs.push_back(mad);
                }
                // r7.x += att0
                {
                    auto add = Instr(D3D10_SB_OPCODE_ADD);
                    add.operands = { TempOp(7, 0x1), Swz(CBOp(0, lb + LR_ATT), 0), Swz(TempOp(7), 0) };
                    out.instrs.push_back(add);
                }
                // r5.x = 1/att_denom  (attenuation factor)
                {
                    auto rcp = Instr(D3D11_SB_OPCODE_RCP);
                    rcp.operands = { TempOp(5, 0x1), Swz(TempOp(7), 0) };
                    out.instrs.push_back(rcp);
                }
                // Clamp att to [0, inf) (division can go negative for very large dists)
                {
                    auto mx = Instr(D3D10_SB_OPCODE_MAX);
                    mx.operands = { TempOp(5, 0x1), Swz(TempOp(5), 0), ImmF(0, 0, 0, 0) };
                    out.instrs.push_back(mx);
                }

                if (lt == D3DLIGHT_SPOT)
                {
                    // cos_rho = dot(-L, lightDir) = -dot(L, lightDir)
                    // LR_DIRECTION stores normalize(dir) in eye space for spot
                    {
                        auto dp3 = Instr(D3D10_SB_OPCODE_DP3);
                        dp3.operands = { TempOp(6, 0x2), TempOp(4), CBOp(0, lb + LR_DIRECTION) };
                        out.instrs.push_back(dp3);
                    }
                    // r6.y = -dot(L, dir) = cos_rho
                    {
                        auto mul = Instr(D3D10_SB_OPCODE_MUL);
                        mul.operands = { TempOp(6, 0x2), Swz(TempOp(6), 1), ImmF(-1, -1, -1, -1) };
                        out.instrs.push_back(mul);
                    }
                    // spot_factor = ((cos_rho - cos(phi/2)) / (cos(theta/2) - cos(phi/2)))^falloff
                    // r7.x = cos_rho - cos(phi/2)
                    {
                        auto add = Instr(D3D10_SB_OPCODE_ADD);
                        add.operands = { TempOp(7, 0x1), Swz(TempOp(6), 1), Neg(Swz(CBOp(0, lb + LR_SPOT), 1)) };
                        out.instrs.push_back(add);
                    }
                    // r7.y = cos(theta/2) - cos(phi/2)
                    {
                        auto add = Instr(D3D10_SB_OPCODE_ADD);
                        add.operands = { TempOp(7, 0x2), Swz(CBOp(0, lb + LR_SPOT), 0), Neg(Swz(CBOp(0, lb + LR_SPOT), 1)) };
                        out.instrs.push_back(add);
                    }
                    // r7.x = (cos_rho - B) / (A - B)   (spot cone fraction)
                    {
                        auto div = Instr(D3D10_SB_OPCODE_DIV);
                        div.operands = { TempOp(7, 0x1), Swz(TempOp(7), 0), Swz(TempOp(7), 1) };
                        out.instrs.push_back(div);
                    }
                    // Clamp to [0, 1]
                    {
                        auto mx = Instr(D3D10_SB_OPCODE_MAX);
                        mx.operands = { TempOp(7, 0x1), Swz(TempOp(7), 0), ImmF(0, 0, 0, 0) };
                        out.instrs.push_back(mx);
                    }
                    {
                        auto mn = Instr(D3D10_SB_OPCODE_MIN);
                        mn.operands = { TempOp(7, 0x1), Swz(TempOp(7), 0), ImmF(1, 1, 1, 1) };
                        out.instrs.push_back(mn);
                    }
                    // spot_factor = r7.x^falloff = exp2(falloff * log2(r7.x))
                    // log2(0) → -inf → exp2(-inf) = 0 (handled correctly by IEEE)
                    {
                        auto lg = Instr(D3D10_SB_OPCODE_LOG);
                        lg.operands = { TempOp(7, 0x1), Swz(TempOp(7), 0) };
                        out.instrs.push_back(lg);
                    }
                    {
                        auto mul = Instr(D3D10_SB_OPCODE_MUL);
                        mul.operands = { TempOp(7, 0x1), Swz(TempOp(7), 0), Swz(CBOp(0, lb + LR_SPOT), 2) };
                        out.instrs.push_back(mul);
                    }
                    {
                        auto ep = Instr(D3D10_SB_OPCODE_EXP);
                        ep.operands = { TempOp(7, 0x1), Swz(TempOp(7), 0) };
                        out.instrs.push_back(ep);
                    }
                    {
                        auto mx = Instr(D3D10_SB_OPCODE_MAX);
                        mx.operands = { TempOp(7, 0x1), Swz(TempOp(7), 0), ImmF(0, 0, 0, 0) };
                        out.instrs.push_back(mx);
                    }
                    // att *= spot_factor
                    {
                        auto mul = Instr(D3D10_SB_OPCODE_MUL);
                        mul.operands = { TempOp(5, 0x1), Swz(TempOp(5), 0), Swz(TempOp(7), 0) };
                        out.instrs.push_back(mul);
                    }
                }
            }

            // r2 += att * light.ambient * mat.ambient
            {
                SM4Operand matAmb = ResolveMatSrc(cv, key.ambientSrc, diffIn, specIn, CB_MAT_AMB);
                auto mul = Instr(D3D10_SB_OPCODE_MUL);
                mul.operands = { TempOp(7), CBOp(0, lb + LR_AMBIENT), matAmb };
                out.instrs.push_back(mul);

                auto mul2 = Instr(D3D10_SB_OPCODE_MUL);
                mul2.operands = { TempOp(7), TempOp(7), Swz(TempOp(5), 0) };
                out.instrs.push_back(mul2);

                auto add = Instr(D3D10_SB_OPCODE_ADD);
                add.operands = { TempOp(2), TempOp(2), TempOp(7) };
                out.instrs.push_back(add);
            }

            // NdotL = max(0, dot3(r1, r4))
            {
                auto dp3 = Instr(D3D10_SB_OPCODE_DP3);
                dp3.operands = { TempOp(6, 0x1), TempOp(1), TempOp(4) };
                out.instrs.push_back(dp3);

                auto mx = Instr(D3D10_SB_OPCODE_MAX);
                mx.operands = { TempOp(6, 0x1), Swz(TempOp(6), 0), ImmF(0, 0, 0, 0) };
                out.instrs.push_back(mx);
            }

            // r2 += att * NdotL * light.diffuse * mat.diffuse
            {
                SM4Operand matDiff = ResolveMatSrc(cv, key.diffuseSrc, diffIn, specIn, CB_MAT_DIFF);
                auto mul = Instr(D3D10_SB_OPCODE_MUL);
                mul.operands = { TempOp(7), CBOp(0, lb + LR_DIFFUSE), matDiff };
                out.instrs.push_back(mul);

                auto mul2 = Instr(D3D10_SB_OPCODE_MUL);
                mul2.operands = { TempOp(7), TempOp(7), Swz(TempOp(6), 0) };
                out.instrs.push_back(mul2);

                auto mul3 = Instr(D3D10_SB_OPCODE_MUL);
                mul3.operands = { TempOp(7), TempOp(7), Swz(TempOp(5), 0) };
                out.instrs.push_back(mul3);

                auto add = Instr(D3D10_SB_OPCODE_ADD);
                add.operands = { TempOp(2), TempOp(2), TempOp(7) };
                out.instrs.push_back(add);
            }

            if (specularOn)
            {
                // H = normalize(L + (0, 0, -1))  — non-local viewer (eye dir = -Z in eye space)
                {
                    auto add = Instr(D3D10_SB_OPCODE_ADD);
                    add.operands = { TempOp(7, 0x7), TempOp(4), ImmF(0, 0, -1, 0) };
                    out.instrs.push_back(add);
                }
                // Normalize r7
                {
                    auto dp3 = Instr(D3D10_SB_OPCODE_DP3);
                    dp3.operands = { TempOp(6, 0x2), TempOp(7), TempOp(7) };
                    out.instrs.push_back(dp3);

                    auto rsq = Instr(D3D10_SB_OPCODE_RSQ);
                    rsq.operands = { TempOp(6, 0x2), Swz(TempOp(6), 1) };
                    out.instrs.push_back(rsq);

                    auto mul = Instr(D3D10_SB_OPCODE_MUL);
                    mul.operands = { TempOp(7, 0x7), TempOp(7), Swz(TempOp(6), 1) };
                    out.instrs.push_back(mul);
                }
                // NdotH = max(0, dot3(r1, r7))
                {
                    auto dp3 = Instr(D3D10_SB_OPCODE_DP3);
                    dp3.operands = { TempOp(6, 0x2), TempOp(1), TempOp(7) };
                    out.instrs.push_back(dp3);

                    auto mx = Instr(D3D10_SB_OPCODE_MAX);
                    mx.operands = { TempOp(6, 0x2), Swz(TempOp(6), 1), ImmF(0, 0, 0, 0) };
                    out.instrs.push_back(mx);
                }
                // spec_factor = NdotH^power = exp2(power * log2(NdotH))
                {
                    auto lg = Instr(D3D10_SB_OPCODE_LOG);
                    lg.operands = { TempOp(6, 0x2), Swz(TempOp(6), 1) };
                    out.instrs.push_back(lg);

                    auto mul = Instr(D3D10_SB_OPCODE_MUL);
                    mul.operands = { TempOp(6, 0x2), Swz(TempOp(6), 1), Swz(CBOp(0, CB_MAT_POWER), 0) };
                    out.instrs.push_back(mul);

                    auto ep = Instr(D3D10_SB_OPCODE_EXP);
                    ep.operands = { TempOp(6, 0x2), Swz(TempOp(6), 1) };
                    out.instrs.push_back(ep);

                    auto mx = Instr(D3D10_SB_OPCODE_MAX);
                    mx.operands = { TempOp(6, 0x2), Swz(TempOp(6), 1), ImmF(0, 0, 0, 0) };
                    out.instrs.push_back(mx);
                }
                // r3 += att * spec_factor * light.specular * mat.specular
                {
                    SM4Operand matSpec = ResolveMatSrc(cv, key.specularSrc, diffIn, specIn, CB_MAT_SPEC);
                    auto mul = Instr(D3D10_SB_OPCODE_MUL);
                    mul.operands = { TempOp(7), CBOp(0, lb + LR_SPECULAR), matSpec };
                    out.instrs.push_back(mul);

                    auto mul2 = Instr(D3D10_SB_OPCODE_MUL);
                    mul2.operands = { TempOp(7), TempOp(7), Swz(TempOp(6), 1) };
                    out.instrs.push_back(mul2);

                    auto mul3 = Instr(D3D10_SB_OPCODE_MUL);
                    mul3.operands = { TempOp(7), TempOp(7), Swz(TempOp(5), 0) };
                    out.instrs.push_back(mul3);

                    auto add = Instr(D3D10_SB_OPCODE_ADD);
                    add.operands = { TempOp(3), TempOp(3), TempOp(7) };
                    out.instrs.push_back(add);
                }
            }
        }

        // Output COLOR0 = saturate(r2)
        {
            auto mv = InstrSat(D3D10_SB_OPCODE_MOV);
            mv.operands = { OutputOp(colorOut), TempOp(2) };
            out.instrs.push_back(mv);
        }

        if (specularOn)
        {
            auto mv = InstrSat(D3D10_SB_OPCODE_MOV);
            mv.operands = { OutputOp(specOut), TempOp(3) };
            out.instrs.push_back(mv);
        }
    }
    else
    {
        // No lighting: pass through vertex diffuse
        if (hasDiffuseFVF && diffIn != 0xFFu && colorOut != 0xFFu)
        {
            auto mv = Instr(D3D10_SB_OPCODE_MOV);
            mv.operands = { OutputOp(colorOut), InputOp(diffIn) };
            out.instrs.push_back(mv);
        }
    }

    for (Uint t = 0; t < numTex && t < 8; ++t)
    {
        if (tcIn[t] == 0xFFu)
        {
            continue;
        }
        DWORD ttf = key.texTransformFlags[t];
        DWORD count = ttf & 0xF;          
        Bool projected = (ttf & 0x80) != 0; 
        if (count == 0 || count > 4)
        {
            auto mv = Instr(D3D10_SB_OPCODE_MOV);
            mv.operands = { OutputOp(tcOut[t], 0x3), InputOp(tcIn[t]) };
            out.instrs.push_back(mv);
        }
        else
        {
            const Uint32 scr = tcXformScratch;
            const Uint32 matBase = CB_TEX_XFORM_BASE + t * 4u;

            { auto mv = Instr(D3D10_SB_OPCODE_MOV); mv.operands = { TempOp(scr, 0x3), InputOp(tcIn[t]) }; out.instrs.push_back(mv); }
            { auto mv = Instr(D3D10_SB_OPCODE_MOV); mv.operands = { TempOp(scr, 0x4), ImmF(0,0,0,0) };    out.instrs.push_back(mv); }
            { auto mv = Instr(D3D10_SB_OPCODE_MOV); mv.operands = { TempOp(scr, 0x8), ImmF(1,1,1,1) };    out.instrs.push_back(mv); }

            for (Uint col = 0; col < count; ++col)
            {
                EmitDP4Col(out, OutputOp(tcOut[t], 1u << col), TempOp(scr), matBase + col);
            }

            if ((ttf & D3DTTFF_PROJECTED) && count >= 2)
            {
                Uint8 wComp = static_cast<Uint8>(count - 1);  // index of divisor component
                SM4Operand divisor = Swz(OutputOp(tcOut[t]), wComp);
                SM4Operand xyDst = OutputOp(tcOut[t], 0x3);
                auto div = Instr(D3D10_SB_OPCODE_DIV);
                div.operands = { xyDst, OutputOp(tcOut[t]), divisor };
                out.instrs.push_back(div);
            }
        }
    }

    if (fogOut != 0xFFu)
    {
        SM4Operand fogOutX  = OutputOp(fogOut, 0x1);
        SM4Operand fogParam = CBOp(0, CB_FOG_PARAMS);

        SM4Operand eyeZ;
        if (lightingOn)
        {
            eyeZ = Swz(TempOp(0), 2);  // r0.z already holds eye-space Z
        }
        else
        {
            // Compute eye-space Z = dot(pos, WV_col2) into r0.x
            EmitDP4Col(out, TempOp(0, 0x1), InputOp(posIn), CB_WV_COL0 + 2);
            eyeZ = Swz(TempOp(0), 0);
        }

        // Scratch: r7.x for lit path (r7 free after lighting), r0.y for non-lit
        Uint32 scrR = lightingOn ? 7u : 0u;
        Uint8  scrC = lightingOn ? 0u : 1u;
        SM4Operand scrDst = TempOp(scrR, 1u << scrC);
        SM4Operand scrSrc = Swz(TempOp(scrR), scrC);
        switch (key.fogVertexMode)
        {
        case D3DFOG_LINEAR:
        {
            auto sub = Instr(D3D10_SB_OPCODE_ADD);
            sub.operands = { scrDst, Swz(fogParam, 1), Neg(eyeZ) };
            out.instrs.push_back(sub);
            auto mul = Instr(D3D10_SB_OPCODE_MUL);
            mul.saturate = true;
            mul.operands = { fogOutX, scrSrc, Swz(fogParam, 3) };
            out.instrs.push_back(mul);
            break;
        }
        case D3DFOG_EXP:
        {
            auto mul = Instr(D3D10_SB_OPCODE_MUL);
            mul.operands = { scrDst, Neg(eyeZ), Swz(fogParam, 2) };
            out.instrs.push_back(mul);
            auto exp = Instr(D3D10_SB_OPCODE_EXP);
            exp.saturate = true;
            exp.operands = { fogOutX, scrSrc };
            out.instrs.push_back(exp);
            break;
        }
        case D3DFOG_EXP2:
        {
            auto mul1 = Instr(D3D10_SB_OPCODE_MUL);
            mul1.operands = { scrDst, Swz(fogParam, 2), eyeZ };
            out.instrs.push_back(mul1);
            auto mul2 = Instr(D3D10_SB_OPCODE_MUL);
            mul2.operands = { scrDst, scrSrc, scrSrc };
            out.instrs.push_back(mul2);
            auto mul3 = Instr(D3D10_SB_OPCODE_MUL);
            mul3.operands = { scrDst, Neg(scrSrc), ImmF(1.4427f, 1.4427f, 1.4427f, 1.4427f) };
            out.instrs.push_back(mul3);
            auto exp = Instr(D3D10_SB_OPCODE_EXP);
            exp.saturate = true;
            exp.operands = { fogOutX, scrSrc };
            out.instrs.push_back(exp);
            break;
        }
        default:
            break;
        }
    }
}

static Bool ArgNeedsTexture(DWORD arg)
{
    return (arg & 0xF) == D3DTA_TEXTURE;
}

// Emits instructions to resolve a TSS arg into a destination temp register.
// Returns an operand for reading the resolved value.
// Uses r_tex (scratch) for texture samples, r_curr for D3DTA_CURRENT.
// diff_in: input reg for diffuse (0xFF = unavailable).
// tc_in:   input reg for texcoord at this stage (0xFF = unavailable).
// sampled: set to true if a SAMPLE was emitted.
static SM4Operand ResolveArg(D3DSW_ParsedShader& out, DWORD arg,
                              Uint32 r_tex, Uint32 r_curr, Bool currInit,
                              Uint32 diffIn, Uint32 tcIn, Uint32 stage,
                              Bool stageHasTex, Bool& sampled)
{
    const Bool complement  = (arg & D3DTA_COMPLEMENT) != 0;
    const Bool alphaRep    = (arg & D3DTA_ALPHAREPLICATE) != 0;
    SM4Operand base{};
    switch (arg & 0xF)
    {
    case D3DTA_TEXTURE:
        if (!sampled && tcIn != 0xFFu && stageHasTex)
        {
            auto samp = Instr(D3D10_SB_OPCODE_SAMPLE);
            samp.operands = { TempOp(r_tex), InputOp(tcIn),
                              ResourceOp(stage), SamplerOp(stage) };
            out.instrs.push_back(samp);
            sampled = true;
        }
        base = sampled ? TempOp(r_tex) : ImmF(1, 1, 1, 1);
        break;
    case D3DTA_DIFFUSE:
    case D3DTA_CURRENT:
        if ((arg & 0xF) == D3DTA_CURRENT && currInit)
        {
            base = TempOp(r_curr);
        }
        else
        {
            base = (diffIn != 0xFFu) ? InputOp(diffIn) : ImmF(0, 0, 0, 0);
        }
        break;
    case D3DTA_SPECULAR:
        base = ImmF(0, 0, 0, 0);
        break;
    case D3DTA_TFACTOR:
        base = ImmF(0.5f, 0.5f, 0.5f, 0.5f);
        break;
    default:
        base = ImmF(0, 0, 0, 0);
        break;
    }

    if (alphaRep)
    {
        base = Swz(base, 3);  
    }

    if (complement)
    {
        auto add = Instr(D3D10_SB_OPCODE_ADD);
        add.operands = { TempOp(r_tex), ImmF(1, 1, 1, 1), Neg(base) };
        out.instrs.push_back(add);
        return TempOp(r_tex);
    }
    return base;
}

void SynthesizeFFPS(const FFPSKey& key, D3DSW_ParsedShader& out)
{
    out = {};
    out.type = D3DSW_ShaderType::Pixel;
    Uint32 numActiveStages = 0;
    Bool needsDiffuse  = key.hasDiffuse != 0;
    Uint8 needsTCForStage[8] = {};
    for (Uint s = 0; s < 8; ++s)
    {
        const auto& sk = key.stages[s];
        if (sk.colorOp == D3DTOP_DISABLE) 
        { 
            break; 
        }

        ++numActiveStages;
        Bool texUsed = (ArgNeedsTexture(sk.colorArg1) || ArgNeedsTexture(sk.colorArg2) ||
                        ArgNeedsTexture(sk.alphaArg1) || ArgNeedsTexture(sk.alphaArg2));
        if (texUsed && sk.hasTexture && s < key.numTexCoords) 
        { 
            needsTCForStage[s] = 1; 
        }
    }

    Uint32 nextIn   = 0;
    Uint32 diffIn   = 0xFFu;
    Uint32 tcIn[8]; 
    for (Uint s = 0; s < 8; ++s) tcIn[s] = 0xFFu;

    if (needsDiffuse)
    {
        diffIn = nextIn;
        out.inputs.push_back(MakeSig("COLOR", 0, nextIn++, 0xF, 0));
    }
    for (Uint s = 0; s < key.numTexCoords && s < 8; ++s)
    {
        tcIn[s] = nextIn;
        out.inputs.push_back(MakeSig("TEXCOORD", s, nextIn++, 0x3, 0));
    }

    out.outputs.push_back(MakeSig("SV_Target", 0, 0, 0xF, D3D_NAME_TARGET));
    for (Uint s = 0; s < 8; ++s)
    {
        if (!needsTCForStage[s]) 
        { 
            continue; 
        }
        D3DSW_TexBinding tb{}; 
        tb.slot = s; 
        tb.dimension = D3D_SRV_DIMENSION_TEXTURE2D;
        out.textures.push_back(tb);
    }

    //r0 = current (running output)
    //r1 = texture sample scratch for current stage
    out.numTemps = (numActiveStages > 0) ? 2u : 0u;
    Bool currInit = false;
    for (Uint s = 0; s < numActiveStages; ++s)
    {
        const auto& sk = key.stages[s];
        Bool sampled = false;
        const Bool stageHasTex = (needsTCForStage[s] != 0);

        SM4Operand arg1c = ResolveArg(out, sk.colorArg1, 1, 0, currInit, diffIn, tcIn[s], s, stageHasTex, sampled);
        SM4Operand arg2c = ResolveArg(out, sk.colorArg2, 1, 0, currInit, diffIn, tcIn[s], s, stageHasTex, sampled);
        auto emitColorOp = [&](SM4Operand a, SM4Operand b, Uint8 mask)
        {
            switch (sk.colorOp)
            {
            case D3DTOP_SELECTARG1:
            {
                auto mv = Instr(D3D10_SB_OPCODE_MOV);
                mv.operands = { TempOp(0, mask), a };
                out.instrs.push_back(mv);
                break;
            }
            case D3DTOP_SELECTARG2:
            {
                auto mv = Instr(D3D10_SB_OPCODE_MOV);
                mv.operands = { TempOp(0, mask), b };
                out.instrs.push_back(mv);
                break;
            }
            case D3DTOP_MODULATE:
            {
                auto mul = Instr(D3D10_SB_OPCODE_MUL);
                mul.operands = { TempOp(0, mask), a, b };
                out.instrs.push_back(mul);
                break;
            }
            case D3DTOP_MODULATE2X:
            {
                auto mul = Instr(D3D10_SB_OPCODE_MUL);
                mul.operands = { TempOp(0, mask), a, b };
                out.instrs.push_back(mul);
                // x2: add to itself, saturate
                auto add = InstrSat(D3D10_SB_OPCODE_ADD);
                add.operands = { TempOp(0, mask), TempOp(0), TempOp(0) };
                out.instrs.push_back(add);
                break;
            }
            case D3DTOP_ADD:
            {
                auto add = InstrSat(D3D10_SB_OPCODE_ADD);
                add.operands = { TempOp(0, mask), a, b };
                out.instrs.push_back(add);
                break;
            }
            case D3DTOP_ADDSIGNED:
            {
                // result = saturate(a + b - 0.5)
                auto add = Instr(D3D10_SB_OPCODE_ADD);
                add.operands = { TempOp(0, mask), a, b };
                out.instrs.push_back(add);
                auto sub = InstrSat(D3D10_SB_OPCODE_ADD);
                sub.operands = { TempOp(0, mask), TempOp(0), ImmF(-0.5f, -0.5f, -0.5f, -0.5f) };
                out.instrs.push_back(sub);
                break;
            }
            case D3DTOP_SUBTRACT:
            {
                auto add = InstrSat(D3D10_SB_OPCODE_ADD);
                add.operands = { TempOp(0, mask), a, Neg(b) };
                out.instrs.push_back(add);
                break;
            }
            case D3DTOP_LERP:
            {
                auto mv = Instr(D3D10_SB_OPCODE_MOV);
                mv.operands = { TempOp(0, mask), a };
                out.instrs.push_back(mv);
                break;
            }
            default:
            {
                // Unknown op: pass arg1 through
                auto mv = Instr(D3D10_SB_OPCODE_MOV);
                mv.operands = { TempOp(0, mask), a };
                out.instrs.push_back(mv);
                break;
            }
            }
        };

        emitColorOp(arg1c, arg2c, 0x7);  

        const DWORD aOp   = sk.alphaOp;
        const DWORD aArg1 = sk.alphaArg1;
        const DWORD aArg2 = sk.alphaArg2;
        if (aOp != D3DTOP_DISABLE)
        {
            Bool sampledAlpha = sampled;  
            SM4Operand arg1a = ResolveArg(out, aArg1, 1, 0, currInit, diffIn, tcIn[s], s, stageHasTex, sampledAlpha);
            SM4Operand arg2a = ResolveArg(out, aArg2, 1, 0, currInit, diffIn, tcIn[s], s, stageHasTex, sampledAlpha);
            arg1a = Swz(arg1a, 3);
            arg2a = Swz(arg2a, 3);
            switch (aOp)
            {
            case D3DTOP_SELECTARG1:
            {
                auto mv = Instr(D3D10_SB_OPCODE_MOV);
                mv.operands = { TempOp(0, 0x8), arg1a };
                out.instrs.push_back(mv);
                break;
            }
            case D3DTOP_SELECTARG2:
            {
                auto mv = Instr(D3D10_SB_OPCODE_MOV);
                mv.operands = { TempOp(0, 0x8), arg2a };
                out.instrs.push_back(mv);
                break;
            }
            case D3DTOP_MODULATE:
            {
                auto mul = Instr(D3D10_SB_OPCODE_MUL);
                mul.operands = { TempOp(0, 0x8), arg1a, arg2a };
                out.instrs.push_back(mul);
                break;
            }
            case D3DTOP_ADD:
            {
                auto add = InstrSat(D3D10_SB_OPCODE_ADD);
                add.operands = { TempOp(0, 0x8), arg1a, arg2a };
                out.instrs.push_back(add);
                break;
            }
            default:
            {
                auto mv = Instr(D3D10_SB_OPCODE_MOV);
                mv.operands = { TempOp(0, 0x8), arg1a };
                out.instrs.push_back(mv);
                break;
            }
            }
        }
        else
        {
            SM4Operand arg1aFb = Swz(arg1c, 3);
            auto mv = Instr(D3D10_SB_OPCODE_MOV);
            mv.operands = { TempOp(0, 0x8), arg1aFb };
            out.instrs.push_back(mv);
        }

        currInit = true;
    }

    if (currInit)
    {
        auto mv = Instr(D3D10_SB_OPCODE_MOV);
        mv.operands = { OutputOp(0), TempOp(0) };
        out.instrs.push_back(mv);
    }
    else if (needsDiffuse && diffIn != 0xFFu)
    {
        auto mv = Instr(D3D10_SB_OPCODE_MOV);
        mv.operands = { OutputOp(0), InputOp(diffIn) };
        out.instrs.push_back(mv);
    }
    else
    {
        auto mv = Instr(D3D10_SB_OPCODE_MOV);
        mv.operands = { OutputOp(0), ImmF(0.5f, 0.5f, 0.5f, 1.0f) };
        out.instrs.push_back(mv);
    }
}

}
