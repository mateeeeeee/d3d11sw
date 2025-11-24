#include "shaders/shader_codegen.h"
#include <sstream>
#include <cstring>

namespace d3d11sw {

const Char* ShaderCodeGen::Comp(Int i)
{
    static const Char* n[] = {"x","y","z","w"};
    return n[i & 3];
}

Uint8 ShaderCodeGen::FindSVPositionReg(const D3D11SW_ParsedShader& shader) const
{
    for (const auto& e : shader.outputs)
    {
        if (e.svType == D3D_NAME_POSITION)
        {
            return static_cast<Uint8>(e.reg);
        }
    }
    return 0xFF;
}

std::string ShaderCodeGen::EmitDstBase(const SM4Operand& op) const
{
    std::ostringstream s;
    switch (op.type)
    {
        case D3D10_SB_OPERAND_TYPE_TEMP:
            s << "r[" << op.indices[0] << "]";
            break;
        case D3D10_SB_OPERAND_TYPE_OUTPUT:
            s << "out_v[" << op.indices[0] << "]";
            break;
        case D3D10_SB_OPERAND_TYPE_OUTPUT_DEPTH:
            s << "out_ptr->oDepth";
            break;
        case D3D11_SB_OPERAND_TYPE_UNORDERED_ACCESS_VIEW:
            s << "res->uav[" << op.indices[0] << "]";
            break;
        default:
            s << "r[0]";
            break;
    }
    return s.str();
}

std::string ShaderCodeGen::EmitSrc(const SM4Operand& op) const
{
    std::ostringstream s;

    std::string base;
    {
        std::ostringstream b;
        switch (op.type)
        {
            case D3D10_SB_OPERAND_TYPE_TEMP:
                b << "r[" << op.indices[0] << "]";
                break;
            case D3D10_SB_OPERAND_TYPE_INPUT:
                b << "in_ptr->v[" << op.indices[0] << "]";
                break;
            case D3D10_SB_OPERAND_TYPE_OUTPUT:
                b << "out_v[" << op.indices[0] << "]";
                break;
            case D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER:
                b << "res->cb[" << op.indices[0] << "][" << op.indices[1] << "]";
                break;
            case D3D11_SB_OPERAND_TYPE_INPUT_THREAD_ID:
                b << "_tid";
                break;
            case D3D11_SB_OPERAND_TYPE_INPUT_THREAD_GROUP_ID:
                b << "_gid";
                break;
            case D3D11_SB_OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP:
                b << "_gtid";
                break;
            case D3D10_SB_OPERAND_TYPE_IMMEDIATE32:
            {
                unsigned bits[4];
                std::memcpy(bits, op.imm, 16);
                char buf[256];
                std::snprintf(buf, sizeof(buf),
                    "SW_float4{sw_uint_bits(0x%08xu),sw_uint_bits(0x%08xu),"
                    "sw_uint_bits(0x%08xu),sw_uint_bits(0x%08xu)}",
                    bits[0], bits[1], bits[2], bits[3]);
                b << buf;
                break;
            }
            default:
                b << "SW_float4{0,0,0,0}";
                break;
        }
        base = b.str();
    }

    bool identitySwizzle = (op.swizzle[0]==0 && op.swizzle[1]==1 &&
                            op.swizzle[2]==2 && op.swizzle[3]==3);

    std::string result;
    if (op.type == D3D10_SB_OPERAND_TYPE_IMMEDIATE32 || identitySwizzle)
    {
        result = base;
    }
    else
    {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "sw_swizzle(%s,%d,%d,%d,%d)",
                      base.c_str(),
                      op.swizzle[0], op.swizzle[1], op.swizzle[2], op.swizzle[3]);
        result = buf;
    }

    if (op.absolute) { result = "sw_abs4(" + result + ")"; }
    if (op.negate)   { result = "(-(" + result + "))"; }

    return result;
}

std::string ShaderCodeGen::EmitWrite(const std::string& dstBase, Uint8 mask,
                                      const std::string& srcExpr, Bool sat) const
{
    std::ostringstream s;
    s << "    { SW_float4 _t = " << srcExpr << ";\n";
    for (Int i = 0; i < 4; ++i)
    {
        if (!(mask & (1 << i))) { continue; }
        s << "      " << dstBase << "." << Comp(i) << " = ";
        if (sat) { s << "sw_saturate("; }
        s << "_t." << Comp(i);
        if (sat) { s << ")"; }
        s << ";\n";
    }
    s << "    }\n";
    return s.str();
}

std::string ShaderCodeGen::EmitInstr(const SM4Instruction& instr,
                                      const D3D11SW_ParsedShader& shader) const
{
    std::ostringstream s;

    const SM4Operand* dst  = instr.operands.size() > 0 ? &instr.operands[0] : nullptr;
    const SM4Operand* src0 = instr.operands.size() > 1 ? &instr.operands[1] : nullptr;
    const SM4Operand* src1 = instr.operands.size() > 2 ? &instr.operands[2] : nullptr;
    const SM4Operand* src2 = instr.operands.size() > 3 ? &instr.operands[3] : nullptr;

    std::string dstBase = dst ? EmitDstBase(*dst) : "r[0]";
    Uint8 mask = dst ? (dst->writeMask ? dst->writeMask : 0xF) : 0xF;
    Bool  sat  = instr.saturate;

    switch (instr.op)
    {
        case D3D10_SB_OPCODE_MOV:
            if (src0) { s << EmitWrite(dstBase, mask, EmitSrc(*src0), sat); }
            break;

        case D3D10_SB_OPCODE_ADD:
            if (src0 && src1)
            {
                s << EmitWrite(dstBase, mask,
                    EmitSrc(*src0) + " + " + EmitSrc(*src1), sat);
            }
            break;

        case D3D10_SB_OPCODE_MUL:
            if (src0 && src1)
            {
                s << EmitWrite(dstBase, mask,
                    EmitSrc(*src0) + " * " + EmitSrc(*src1), sat);
            }
            break;

        case D3D10_SB_OPCODE_MAD:
            if (src0 && src1 && src2)
            {
                s << EmitWrite(dstBase, mask,
                    EmitSrc(*src0) + " * " + EmitSrc(*src1) + " + " + EmitSrc(*src2), sat);
            }
            break;

        case D3D10_SB_OPCODE_DIV:
            if (src0 && src1)
            {
                s << EmitWrite(dstBase, mask,
                    EmitSrc(*src0) + " / " + EmitSrc(*src1), sat);
            }
            break;

        case D3D10_SB_OPCODE_DP2:
            if (src0 && src1)
            {
                std::string expr = "SW_float4{sw_dot2(" + EmitSrc(*src0) + "," + EmitSrc(*src1) + ")}";
                s << EmitWrite(dstBase, mask, expr, sat);
            }
            break;

        case D3D10_SB_OPCODE_DP3:
            if (src0 && src1)
            {
                std::string scalar = "sw_dot3(" + EmitSrc(*src0) + "," + EmitSrc(*src1) + ")";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "    " << dstBase << "." << Comp(i) << " = ";
                    if (sat) { s << "sw_saturate("; }
                    s << scalar;
                    if (sat) { s << ")"; }
                    s << ";\n";
                }
            }
            break;

        case D3D10_SB_OPCODE_DP4:
            if (src0 && src1)
            {
                std::string scalar = "sw_dot4(" + EmitSrc(*src0) + "," + EmitSrc(*src1) + ")";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "    " << dstBase << "." << Comp(i) << " = ";
                    if (sat) { s << "sw_saturate("; }
                    s << scalar;
                    if (sat) { s << ")"; }
                    s << ";\n";
                }
            }
            break;

        case D3D10_SB_OPCODE_MIN:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_min(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_MAX:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_max(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_SQRT:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_sqrt(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_RSQ:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_rsq(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_LOG:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_log2(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_EXP:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_exp2(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_FRC:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_frc(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_ROUND_NE:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_round_ne(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_ROUND_Z:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_round_z(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_FTOI:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_ftoi(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_ITOF:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_itof(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_UTOF:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_utof(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_FTOU:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_ftou(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_IADD:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_iadd(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_ISHL:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_ishl(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_ISHR:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_ishr(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_USHR:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_ushr(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_AND:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_and(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_OR:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_or(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_XOR:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_xor(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_NOT:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_not(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_INEG:
            if (src0)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_ineg(_a." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_IMAX:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_imax(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_IMIN:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_imin(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_UMAX:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_umax(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_UMIN:
            if (src0 && src1)
            {
                s << "    { SW_float4 _a=" << EmitSrc(*src0) << ", _b=" << EmitSrc(*src1) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = sw_umin(_a." << Comp(i) << ",_b." << Comp(i) << ");\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_MOVC:
            if (src0 && src1 && src2)
            {
                s << "    { SW_float4 _c=" << EmitSrc(*src0)
                  << ", _t=" << EmitSrc(*src1)
                  << ", _f=" << EmitSrc(*src2) << ";\n";
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(mask & (1 << i))) { continue; }
                    s << "      " << dstBase << "." << Comp(i)
                      << " = (_c." << Comp(i) << " != 0.f) ? _t." << Comp(i)
                      << " : _f." << Comp(i) << ";\n";
                }
                s << "    }\n";
            }
            break;

        case D3D10_SB_OPCODE_IF:
            if (dst)
            {
                s << "    if ((" << EmitSrc(*dst) << ".x) ";
                s << (instr.testNonZero ? "!= 0.f" : "== 0.f");
                s << ")\n    {\n";
            }
            break;

        case D3D10_SB_OPCODE_ELSE:
            s << "    }\n    else\n    {\n";
            break;

        case D3D10_SB_OPCODE_ENDIF:
            s << "    }\n";
            break;

        case D3D10_SB_OPCODE_LOOP:
            s << "    while (true)\n    {\n";
            break;

        case D3D10_SB_OPCODE_ENDLOOP:
            s << "    }\n";
            break;

        case D3D10_SB_OPCODE_BREAK:
            s << "    break;\n";
            break;

        case D3D10_SB_OPCODE_BREAKC:
            if (dst)
            {
                s << "    if ((" << EmitSrc(*dst) << ".x) ";
                s << (instr.testNonZero ? "!= 0.f" : "== 0.f");
                s << ") { break; }\n";
            }
            break;

        case D3D10_SB_OPCODE_SAMPLE:
            if (dst && src0 && src1 && src2)
            {
                Uint32 texSlot = src1->indices[0];
                Uint32 smpSlot = src2->indices[0];
                std::string uv = EmitSrc(*src0);
                s << EmitWrite(dstBase, mask,
                    "sw_sample_2d(res->tex[" + std::to_string(texSlot) + "],"
                    "res->smp[" + std::to_string(smpSlot) + "],"
                    "(" + uv + ").x,(" + uv + ").y)", sat);
            }
            break;

        case D3D10_SB_OPCODE_SAMPLE_L:
            if (dst && src0 && src1 && src2)
            {
                Uint32 texSlot = src1->indices[0];
                Uint32 smpSlot = src2->indices[0];
                std::string uv = EmitSrc(*src0);
                s << EmitWrite(dstBase, mask,
                    "sw_sample_2d(res->tex[" + std::to_string(texSlot) + "],"
                    "res->smp[" + std::to_string(smpSlot) + "],"
                    "(" + uv + ").x,(" + uv + ").y)", sat);
            }
            break;

        case D3D10_SB_OPCODE_LD:
            if (dst && src0 && src1)
            {
                Uint32 texSlot = src1->indices[0];
                std::string coord = EmitSrc(*src0);
                s << EmitWrite(dstBase, mask,
                    "sw_fetch_texel(res->tex[" + std::to_string(texSlot) + "],"
                    "(unsigned)((" + coord + ").x),"
                    "(unsigned)((" + coord + ").y))", sat);
            }
            break;

        case D3D11_SB_OPCODE_LD_UAV_TYPED:
            if (dst && src0 && src1)
            {
                Uint32 uavSlot = src1->indices[0];
                std::string addr = EmitSrc(*src0);
                s << EmitWrite(dstBase, mask,
                    "sw_uav_load_typed(res->uav[" + std::to_string(uavSlot) + "],"
                    "sw_bits_uint((" + addr + ").x))", sat);
            }
            break;

        case D3D11_SB_OPCODE_STORE_UAV_TYPED:
            if (dst && src0 && src1)
            {
                Uint32 uavSlot = dst->indices[0];
                std::string addr = EmitSrc(*src0);
                std::string val  = EmitSrc(*src1);
                s << "    sw_uav_store_typed(res->uav[" << uavSlot << "],"
                  << "sw_bits_uint((" << addr << ").x)," << val << ");\n";
            }
            break;

        case D3D11_SB_OPCODE_LD_RAW:
            if (dst && src0 && src1)
            {
                Uint32 uavSlot = src1->indices[0];
                std::string addr = EmitSrc(*src0);
                s << EmitWrite(dstBase, mask,
                    "sw_uav_load_raw(res->uav[" + std::to_string(uavSlot) + "],"
                    "sw_bits_uint((" + addr + ").x))", sat);
            }
            break;

        case D3D11_SB_OPCODE_STORE_RAW:
            if (dst && src0 && src1)
            {
                Uint32 uavSlot = dst->indices[0];
                std::string addr = EmitSrc(*src0);
                std::string val  = EmitSrc(*src1);
                s << "    sw_uav_store_raw(res->uav[" << uavSlot << "],"
                  << "sw_bits_uint((" << addr << ").x)," << val << ");\n";
            }
            break;

        case D3D11_SB_OPCODE_LD_STRUCTURED:
            if (dst && src0 && src1 && src2)
            {
                Uint32 uavSlot     = src2->indices[0];
                std::string idx    = EmitSrc(*src0);
                std::string offset = EmitSrc(*src1);
                s << EmitWrite(dstBase, mask,
                    "sw_uav_load_structured(res->uav[" + std::to_string(uavSlot) + "],"
                    "sw_bits_uint((" + idx + ").x),"
                    "sw_bits_uint((" + offset + ").x))", sat);
            }
            break;

        case D3D11_SB_OPCODE_STORE_STRUCTURED:
            if (dst && src0 && src1 && src2)
            {
                Uint32 uavSlot     = dst->indices[0];
                std::string idx    = EmitSrc(*src0);
                std::string offset = EmitSrc(*src1);
                std::string val    = EmitSrc(*src2);
                s << "    sw_uav_store_structured(res->uav[" << uavSlot << "],"
                  << "sw_bits_uint((" << idx << ").x),"
                  << "sw_bits_uint((" << offset << ").x)," << val << ");\n";
            }
            break;

        case D3D10_SB_OPCODE_RET:
        case D3D10_SB_OPCODE_NOP:
        case D3D10_SB_OPCODE_DCL_TEMPS:
        case D3D10_SB_OPCODE_DCL_INPUT:
        case D3D10_SB_OPCODE_DCL_INPUT_PS:
        case D3D10_SB_OPCODE_DCL_INPUT_SGV:
        case D3D10_SB_OPCODE_DCL_INPUT_SIV:
        case D3D10_SB_OPCODE_DCL_INPUT_PS_SGV:
        case D3D10_SB_OPCODE_DCL_INPUT_PS_SIV:
        case D3D10_SB_OPCODE_DCL_OUTPUT:
        case D3D10_SB_OPCODE_DCL_OUTPUT_SGV:
        case D3D10_SB_OPCODE_DCL_OUTPUT_SIV:
        case D3D10_SB_OPCODE_DCL_CONSTANT_BUFFER:
        case D3D10_SB_OPCODE_DCL_RESOURCE:
        case D3D10_SB_OPCODE_DCL_SAMPLER:
        case D3D11_SB_OPCODE_DCL_THREAD_GROUP:
        case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
        case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
        case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
            break;

        default:
            s << "    /* unhandled op=" << (int)instr.op << " */\n";
            break;
    }

    return s.str();
}

std::string ShaderCodeGen::EmitInstructions(const D3D11SW_ParsedShader& shader) const
{
    std::ostringstream s;
    for (const auto& instr : shader.instrs)
    {
        s << EmitInstr(instr, shader);
    }
    return s.str();
}

std::string ShaderCodeGen::EmitVS(const D3D11SW_ParsedShader& shader) const
{
    std::ostringstream s;
    Uint32 numTemps = shader.numTemps > 0 ? shader.numTemps : 1;
    Uint8  svPosReg = FindSVPositionReg(shader);

    s << "extern \"C\" void ShaderMain(const SW_VSInput* in_ptr, SW_VSOutput* out_ptr,"
         " const SW_Resources* res)\n{\n";
    s << "    SW_float4 r[" << numTemps << "] = {};\n";
    s << "    SW_float4 out_v[" << D3D11_VS_OUTPUT_REGISTER_COUNT << "] = {};\n";
    s << "\n";

    s << EmitInstructions(shader);

    s << "\n";
    for (const auto& e : shader.outputs)
    {
        if (e.svType == D3D_NAME_POSITION)
        {
            s << "    out_ptr->pos = out_v[" << e.reg << "];\n";
        }
        else
        {
            s << "    out_ptr->o[" << e.reg << "] = out_v[" << e.reg << "];\n";
        }
    }

    if (svPosReg == 0xFF && !shader.outputs.empty())
    {
        s << "    out_ptr->pos = out_v[0];\n";
    }

    s << "}\n";
    return s.str();
}

std::string ShaderCodeGen::EmitPS(const D3D11SW_ParsedShader& shader) const
{
    std::ostringstream s;
    Uint32 numTemps = shader.numTemps > 0 ? shader.numTemps : 1;

    s << "extern \"C\" void ShaderMain(const SW_PSInput* in_ptr, SW_PSOutput* out_ptr,"
         " const SW_Resources* res)\n{\n";
    s << "    SW_float4 r[" << numTemps << "] = {};\n";
    s << "    SW_float4 out_v[" << D3D11_PS_OUTPUT_REGISTER_COUNT << "] = {};\n";
    s << "    out_ptr->depthWritten = false;\n";
    s << "\n";

    s << EmitInstructions(shader);

    s << "\n";
    for (Uint32 i = 0; i < D3D11_PS_OUTPUT_REGISTER_COUNT; ++i)
    {
        s << "    out_ptr->oC[" << i << "] = out_v[" << i << "];\n";
    }

    s << "}\n";
    return s.str();
}

std::string ShaderCodeGen::EmitCS(const D3D11SW_ParsedShader& shader) const
{
    std::ostringstream s;
    Uint32 numTemps = shader.numTemps > 0 ? shader.numTemps : 1;

    s << "extern \"C\" void ShaderMain(const SW_CSInput* in_ptr,"
         " SW_Resources* res)\n{\n";
    s << "    SW_float4 r[" << numTemps << "] = {};\n";
    s << "    SW_float4 _tid   = { sw_uint_bits(in_ptr->dispatchThreadID.x), sw_uint_bits(in_ptr->dispatchThreadID.y), sw_uint_bits(in_ptr->dispatchThreadID.z), 0.f };\n";
    s << "    SW_float4 _gid   = { sw_uint_bits(in_ptr->groupID.x), sw_uint_bits(in_ptr->groupID.y), sw_uint_bits(in_ptr->groupID.z), 0.f };\n";
    s << "    SW_float4 _gtid  = { sw_uint_bits(in_ptr->groupThreadID.x), sw_uint_bits(in_ptr->groupThreadID.y), sw_uint_bits(in_ptr->groupThreadID.z), 0.f };\n";
    s << "    (void)_tid; (void)_gid; (void)_gtid;\n\n";

    s << EmitInstructions(shader);

    s << "}\n";
    return s.str();
}

std::string ShaderCodeGen::Emit(const D3D11SW_ParsedShader& shader,
                                 const std::string& runtimeHeaderPath) const
{
    std::ostringstream s;
    s << "#include \"" << runtimeHeaderPath << "\"\n\n";

    switch (shader.type)
    {
        case D3D11SW_ShaderType::Vertex:  s << EmitVS(shader); break;
        case D3D11SW_ShaderType::Pixel:   s << EmitPS(shader); break;
        case D3D11SW_ShaderType::Compute: s << EmitCS(shader); break;
        default:
            s << "extern \"C\" void ShaderMain() {}\n";
            break;
    }

    return s.str();
}

}
