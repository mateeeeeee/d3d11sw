#include "shaders/shader_codegen.h"
#include <format>
#include <cstring>

namespace d3d11sw {

namespace {

class CodeWriter
{
public:
    template<typename... ArgsT>
    void Line(std::format_string<ArgsT...> fmt, ArgsT&&... args)
    {
        _buf.append(_depth * 4, ' ');
        std::format_to(std::back_inserter(_buf), fmt, std::forward<ArgsT>(args)...);
        _buf += '\n';
    }

    void Newline() { _buf += '\n'; }
    void Indent()  { ++_depth; }
    void Dedent()  { --_depth; }

    std::string Take() { return std::move(_buf); }

private:
    std::string _buf;
    Int _depth = 0;
};

const Char* Comp(Int i)
{
    static const Char* n[] = {"x","y","z","w"};
    return n[i & 3];
}

Uint8 FindSVPositionReg(const D3D11SW_ParsedShader& shader)
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

std::string EmitDstBase(const SM4Operand& op)
{
    switch (op.type)
    {
    case D3D10_SB_OPERAND_TYPE_TEMP:                        return std::format("r[{}]", op.indices[0]);
    case D3D10_SB_OPERAND_TYPE_OUTPUT:                      return std::format("out_v[{}]", op.indices[0]);
    case D3D10_SB_OPERAND_TYPE_OUTPUT_DEPTH:                return "out_ptr->oDepth";
    case D3D11_SB_OPERAND_TYPE_UNORDERED_ACCESS_VIEW:       return std::format("res->uav[{}]", op.indices[0]);
    case D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY:  return std::format("tgsm[{}]", op.indices[0]);
    default:                                                return "r[0]";
    }
}

std::string EmitSrc(const SM4Operand& op)
{
    std::string base;
    switch (op.type)
    {
    case D3D10_SB_OPERAND_TYPE_TEMP:
        base = std::format("r[{}]", op.indices[0]);
        break;
    case D3D10_SB_OPERAND_TYPE_INPUT:
        base = std::format("in_ptr->v[{}]", op.indices[0]);
        break;
    case D3D10_SB_OPERAND_TYPE_OUTPUT:
        base = std::format("out_v[{}]", op.indices[0]);
        break;
    case D3D10_SB_OPERAND_TYPE_CONSTANT_BUFFER:
        base = std::format("res->cb[{}][{}]", op.indices[0], op.indices[1]);
        break;
    case D3D11_SB_OPERAND_TYPE_INPUT_THREAD_ID:                    base = "_tid";  break;
    case D3D11_SB_OPERAND_TYPE_INPUT_THREAD_GROUP_ID:              base = "_gid";  break;
    case D3D11_SB_OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP:           base = "_gtid"; break;
    case D3D11_SB_OPERAND_TYPE_INPUT_THREAD_ID_IN_GROUP_FLATTENED: base = "_gi";   break;
    case D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY:
        base = std::format("tgsm[{}]", op.indices[0]);
        break;
    case D3D10_SB_OPERAND_TYPE_IMMEDIATE32:
    {
        Uint32 bits[4];
        std::memcpy(bits, op.imm, 16);
        base = std::format(
            "SW_float4{{sw_uint_bits(0x{:08x}u),sw_uint_bits(0x{:08x}u),"
            "sw_uint_bits(0x{:08x}u),sw_uint_bits(0x{:08x}u)}}",
            bits[0], bits[1], bits[2], bits[3]);
        break;
    }
    default:
        base = "SW_float4{0,0,0,0}";
        break;
    }

    Bool identitySwizzle = (op.swizzle[0]==0 && op.swizzle[1]==1 &&
                            op.swizzle[2]==2 && op.swizzle[3]==3);

    std::string result;
    if (op.type == D3D10_SB_OPERAND_TYPE_IMMEDIATE32 || identitySwizzle)
    {
        result = base;
    }
    else
    {
        result = std::format("sw_swizzle({},{},{},{},{})",
                             base, op.swizzle[0], op.swizzle[1], op.swizzle[2], op.swizzle[3]);
    }

    if (op.absolute)
    {
        result = std::format("sw_abs4({})", result);
    }
    if (op.negate)
    {
        if (op.intContext)
        {
            result = std::format("sw_ineg4({})", result);
        }
        else
        {
            result = std::format("(-({}))", result);
        }
    }

    return result;
}

void EmitWrite(CodeWriter& w, std::string_view dst, Uint8 mask,
               std::string_view expr, Bool sat)
{
    w.Line("{{ SW_float4 _t = {};", expr);
    for (Int i = 0; i < 4; ++i)
    {
        if (!(mask & (1 << i)))
        {
            continue;
        }
        if (sat)
        {
            w.Line("  {}.{} = sw_saturate(_t.{});", dst, Comp(i), Comp(i));
        }
        else
        {
            w.Line("  {}.{} = _t.{};", dst, Comp(i), Comp(i));
        }
    }
    w.Line("}}");
}

void EmitPerComp1(CodeWriter& w, const Char* fn,
                  std::string_view dst, Uint8 mask, const SM4Operand& src)
{
    w.Line("{{ SW_float4 _a = {};", EmitSrc(src));
    for (Int i = 0; i < 4; ++i)
    {
        if (!(mask & (1 << i)))
        {
            continue;
        }
        w.Line("  {}.{} = {}(_a.{});", dst, Comp(i), fn, Comp(i));
    }
    w.Line("}}");
}

void EmitPerComp2(CodeWriter& w, const Char* fn,
                  std::string_view dst, Uint8 mask,
                  const SM4Operand& a, const SM4Operand& b)
{
    w.Line("{{ SW_float4 _a = {}, _b = {};", EmitSrc(a), EmitSrc(b));
    for (Int i = 0; i < 4; ++i)
    {
        if (!(mask & (1 << i)))
        {
            continue;
        }
        w.Line("  {}.{} = {}(_a.{}, _b.{});", dst, Comp(i), fn, Comp(i), Comp(i));
    }
    w.Line("}}");
}

void EmitScalarBroadcast(CodeWriter& w, std::string_view dst, Uint8 mask,
                         std::string_view scalar, Bool sat)
{
    for (Int i = 0; i < 4; ++i)
    {
        if (!(mask & (1 << i)))
        {
            continue;
        }
        if (sat)
        {
            w.Line("{}.{} = sw_saturate({});", dst, Comp(i), scalar);
        }
        else
        {
            w.Line("{}.{} = {};", dst, Comp(i), scalar);
        }
    }
}

void EmitInstr(CodeWriter& w, const SM4Instruction& instr,
               const D3D11SW_ParsedShader& shader)
{
    Bool isIntOp = false;
    switch (instr.op)
    {
    case D3D10_SB_OPCODE_IADD: case D3D10_SB_OPCODE_IMUL: case D3D10_SB_OPCODE_IMAD:
    case D3D10_SB_OPCODE_ISHL: case D3D10_SB_OPCODE_ISHR: case D3D10_SB_OPCODE_USHR:
    case D3D10_SB_OPCODE_AND:  case D3D10_SB_OPCODE_OR:   case D3D10_SB_OPCODE_XOR:
    case D3D10_SB_OPCODE_NOT:  case D3D10_SB_OPCODE_INEG:
    case D3D10_SB_OPCODE_IMAX: case D3D10_SB_OPCODE_IMIN:
    case D3D10_SB_OPCODE_UMAX: case D3D10_SB_OPCODE_UMIN:
    case D3D10_SB_OPCODE_IEQ:  case D3D10_SB_OPCODE_INE:
    case D3D10_SB_OPCODE_IGE:  case D3D10_SB_OPCODE_ILT:
    case D3D10_SB_OPCODE_UGE:  case D3D10_SB_OPCODE_ULT:
    case D3D10_SB_OPCODE_UDIV:
        isIntOp = true;
        break;
    default: break;
    }

    auto fixOp = [&](const SM4Operand* op) -> SM4Operand
    {
        SM4Operand copy = op ? *op : SM4Operand{};
        if (isIntOp)
        {
            copy.intContext = true;
        }
        return copy;
    };

    SM4Operand op0 = instr.operands.size() > 0 ? fixOp(&instr.operands[0]) : SM4Operand{};
    SM4Operand op1 = instr.operands.size() > 1 ? fixOp(&instr.operands[1]) : SM4Operand{};
    SM4Operand op2 = instr.operands.size() > 2 ? fixOp(&instr.operands[2]) : SM4Operand{};
    SM4Operand op3 = instr.operands.size() > 3 ? fixOp(&instr.operands[3]) : SM4Operand{};

    const SM4Operand* dst  = instr.operands.size() > 0 ? &op0 : nullptr;
    const SM4Operand* src0 = instr.operands.size() > 1 ? &op1 : nullptr;
    const SM4Operand* src1 = instr.operands.size() > 2 ? &op2 : nullptr;
    const SM4Operand* src2 = instr.operands.size() > 3 ? &op3 : nullptr;

    std::string dstBase = dst ? EmitDstBase(*dst) : "r[0]";
    Uint8 mask = dst ? (dst->writeMask ? dst->writeMask : 0xF) : 0xF;
    Bool sat = instr.saturate;

    switch (instr.op)
    {
    case D3D10_SB_OPCODE_MOV:
        if (src0)
        {
            EmitWrite(w, dstBase, mask, EmitSrc(*src0), sat);
        }
        break;

    case D3D10_SB_OPCODE_ADD:
        if (src0 && src1)
        {
            EmitWrite(w, dstBase, mask, std::format("{} + {}", EmitSrc(*src0), EmitSrc(*src1)), sat);
        }
        break;

    case D3D10_SB_OPCODE_MUL:
        if (src0 && src1)
        {
            EmitWrite(w, dstBase, mask, std::format("{} * {}", EmitSrc(*src0), EmitSrc(*src1)), sat);
        }
        break;

    case D3D10_SB_OPCODE_MAD:
        if (src0 && src1 && src2)
        {
            EmitWrite(w, dstBase, mask,
                std::format("{} * {} + {}", EmitSrc(*src0), EmitSrc(*src1), EmitSrc(*src2)), sat);
        }
        break;

    case D3D10_SB_OPCODE_DIV:
        if (src0 && src1)
        {
            EmitWrite(w, dstBase, mask, std::format("{} / {}", EmitSrc(*src0), EmitSrc(*src1)), sat);
        }
        break;

    case D3D10_SB_OPCODE_DP2:
        if (src0 && src1)
        {
            EmitWrite(w, dstBase, mask,
                std::format("SW_float4{{sw_dot2({},{})}}", EmitSrc(*src0), EmitSrc(*src1)), sat);
        }
        break;

    case D3D10_SB_OPCODE_DP3:
        if (src0 && src1)
        {
            EmitScalarBroadcast(w, dstBase, mask,
                std::format("sw_dot3({},{})", EmitSrc(*src0), EmitSrc(*src1)), sat);
        }
        break;

    case D3D10_SB_OPCODE_DP4:
        if (src0 && src1)
        {
            EmitScalarBroadcast(w, dstBase, mask,
                std::format("sw_dot4({},{})", EmitSrc(*src0), EmitSrc(*src1)), sat);
        }
        break;

    case D3D10_SB_OPCODE_SQRT:     if (src0) { EmitPerComp1(w, "sw_sqrt",     dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_RSQ:      if (src0) { EmitPerComp1(w, "sw_rsq",      dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_LOG:      if (src0) { EmitPerComp1(w, "sw_log2",     dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_EXP:      if (src0) { EmitPerComp1(w, "sw_exp2",     dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_FRC:      if (src0) { EmitPerComp1(w, "sw_frc",      dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_ROUND_NE: if (src0) { EmitPerComp1(w, "sw_round_ne", dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_ROUND_Z:  if (src0) { EmitPerComp1(w, "sw_round_z",  dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_FTOI:     if (src0) { EmitPerComp1(w, "sw_ftoi",     dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_ITOF:     if (src0) { EmitPerComp1(w, "sw_itof",     dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_UTOF:     if (src0) { EmitPerComp1(w, "sw_utof",     dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_FTOU:     if (src0) { EmitPerComp1(w, "sw_ftou",     dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_NOT:      if (src0) { EmitPerComp1(w, "sw_not",      dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_INEG:     if (src0) { EmitPerComp1(w, "sw_ineg",     dstBase, mask, *src0); } break;

    case D3D10_SB_OPCODE_MIN:  if (src0 && src1) { EmitPerComp2(w, "sw_min",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_MAX:  if (src0 && src1) { EmitPerComp2(w, "sw_max",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_IADD: if (src0 && src1) { EmitPerComp2(w, "sw_iadd", dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_ISHL: if (src0 && src1) { EmitPerComp2(w, "sw_ishl", dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_ISHR: if (src0 && src1) { EmitPerComp2(w, "sw_ishr", dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_USHR: if (src0 && src1) { EmitPerComp2(w, "sw_ushr", dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_AND:  if (src0 && src1) { EmitPerComp2(w, "sw_and",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_OR:   if (src0 && src1) { EmitPerComp2(w, "sw_or",   dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_XOR:  if (src0 && src1) { EmitPerComp2(w, "sw_xor",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_IMAX: if (src0 && src1) { EmitPerComp2(w, "sw_imax", dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_IMIN: if (src0 && src1) { EmitPerComp2(w, "sw_imin", dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_UMAX: if (src0 && src1) { EmitPerComp2(w, "sw_umax", dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_UMIN: if (src0 && src1) { EmitPerComp2(w, "sw_umin", dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_IEQ:  if (src0 && src1) { EmitPerComp2(w, "sw_ieq",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_INE:  if (src0 && src1) { EmitPerComp2(w, "sw_ine",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_IGE:  if (src0 && src1) { EmitPerComp2(w, "sw_ige",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_ILT:  if (src0 && src1) { EmitPerComp2(w, "sw_ilt",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_UGE:  if (src0 && src1) { EmitPerComp2(w, "sw_uge",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_ULT:  if (src0 && src1) { EmitPerComp2(w, "sw_ult",  dstBase, mask, *src0, *src1); } break;

    case D3D10_SB_OPCODE_EQ:   if (src0 && src1) { EmitPerComp2(w, "sw_feq",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_NE:   if (src0 && src1) { EmitPerComp2(w, "sw_fne",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_GE:   if (src0 && src1) { EmitPerComp2(w, "sw_fge",  dstBase, mask, *src0, *src1); } break;
    case D3D10_SB_OPCODE_LT:   if (src0 && src1) { EmitPerComp2(w, "sw_flt",  dstBase, mask, *src0, *src1); } break;

    case D3D10_SB_OPCODE_MOVC:
        if (src0 && src1 && src2)
        {
            w.Line("{{ SW_float4 _c = {}, _t = {}, _f = {};",
                   EmitSrc(*src0), EmitSrc(*src1), EmitSrc(*src2));
            for (Int i = 0; i < 4; ++i)
            {
                if (!(mask & (1 << i)))
                {
                    continue;
                }
                w.Line("  {}.{} = (_c.{} != 0.f) ? _t.{} : _f.{};",
                       dstBase, Comp(i), Comp(i), Comp(i), Comp(i));
            }
            w.Line("}}");
        }
        break;

    case D3D10_SB_OPCODE_UDIV:
    {
        const SM4Operand* dstQ = dst;
        const SM4Operand* dstR = src0;
        const SM4Operand* srcA = src1;
        const SM4Operand* srcB = src2;
        if (srcA && srcB)
        {
            w.Line("{{ SW_float4 _a = {}, _b = {};", EmitSrc(*srcA), EmitSrc(*srcB));
            if (dstQ && dstQ->type != D3D10_SB_OPERAND_TYPE_NULL)
            {
                std::string qBase = EmitDstBase(*dstQ);
                Uint8 qMask = dstQ->writeMask ? dstQ->writeMask : 0xF;
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(qMask & (1 << i)))
                    {
                        continue;
                    }
                    w.Line("  {}.{} = sw_udiv(_a.{}, _b.{});", qBase, Comp(i), Comp(i), Comp(i));
                }
            }
            if (dstR && dstR->type != D3D10_SB_OPERAND_TYPE_NULL)
            {
                std::string rBase = EmitDstBase(*dstR);
                Uint8 rMask = dstR->writeMask ? dstR->writeMask : 0xF;
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(rMask & (1 << i)))
                    {
                        continue;
                    }
                    w.Line("  {}.{} = sw_umod(_a.{}, _b.{});", rBase, Comp(i), Comp(i), Comp(i));
                }
            }
            w.Line("}}");
        }
        break;
    }

    case D3D10_SB_OPCODE_IMUL:
    {
        //operands: dst_hi, dst_lo, src_a, src_b
        const SM4Operand* dstHi = dst;
        const SM4Operand* dstLo = src0;
        const SM4Operand* srcA  = src1;
        const SM4Operand* srcB  = src2;
        if (srcA && srcB)
        {
            w.Line("{{ SW_float4 _a = {}, _b = {};", EmitSrc(*srcA), EmitSrc(*srcB));
            if (dstHi && dstHi->type != D3D10_SB_OPERAND_TYPE_NULL)
            {
                std::string hBase = EmitDstBase(*dstHi);
                Uint8 hMask = dstHi->writeMask ? dstHi->writeMask : 0xF;
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(hMask & (1 << i)))
                    {
                        continue;
                    }
                    w.Line("  {}.{} = sw_imul_hi(_a.{}, _b.{});", hBase, Comp(i), Comp(i), Comp(i));
                }
            }
            if (dstLo && dstLo->type != D3D10_SB_OPERAND_TYPE_NULL)
            {
                std::string lBase = EmitDstBase(*dstLo);
                Uint8 lMask = dstLo->writeMask ? dstLo->writeMask : 0xF;
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(lMask & (1 << i)))
                    {
                        continue;
                    }
                    w.Line("  {}.{} = sw_imul_lo(_a.{}, _b.{});", lBase, Comp(i), Comp(i), Comp(i));
                }
            }
            w.Line("}}");
        }
        break;
    }

    case D3D10_SB_OPCODE_SINCOS:
    {
        // operands: dst_sin, dst_cos, src
        const SM4Operand* dstSin = dst;
        const SM4Operand* dstCos = src0;
        const SM4Operand* srcVal = src1;
        if (srcVal)
        {
            w.Line("{{ SW_float4 _a = {};", EmitSrc(*srcVal));
            if (dstSin && dstSin->type != D3D10_SB_OPERAND_TYPE_NULL)
            {
                std::string sBase = EmitDstBase(*dstSin);
                Uint8 sMask = dstSin->writeMask ? dstSin->writeMask : 0xF;
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(sMask & (1 << i)))
                    {
                        continue;
                    }
                    w.Line("  {}.{} = sw_sin(_a.{});", sBase, Comp(i), Comp(i));
                }
            }
            if (dstCos && dstCos->type != D3D10_SB_OPERAND_TYPE_NULL)
            {
                std::string cBase = EmitDstBase(*dstCos);
                Uint8 cMask = dstCos->writeMask ? dstCos->writeMask : 0xF;
                for (Int i = 0; i < 4; ++i)
                {
                    if (!(cMask & (1 << i)))
                    {
                        continue;
                    }
                    w.Line("  {}.{} = sw_cos(_a.{});", cBase, Comp(i), Comp(i));
                }
            }
            w.Line("}}");
        }
        break;
    }

    case D3D10_SB_OPCODE_IF:
        if (dst)
        {
            w.Line("if (({}.x) {} 0.f)", EmitSrc(*dst), instr.testNonZero ? "!=" : "==");
            w.Line("{{");
            w.Indent();
        }
        break;

    case D3D10_SB_OPCODE_ELSE:
        w.Dedent();
        w.Line("}}");
        w.Line("else");
        w.Line("{{");
        w.Indent();
        break;

    case D3D10_SB_OPCODE_ENDIF:
        w.Dedent();
        w.Line("}}");
        break;

    case D3D10_SB_OPCODE_LOOP:
        w.Line("while (true)");
        w.Line("{{");
        w.Indent();
        break;

    case D3D10_SB_OPCODE_ENDLOOP:
        w.Dedent();
        w.Line("}}");
        break;

    case D3D10_SB_OPCODE_BREAK:
        w.Line("break;");
        break;

    case D3D10_SB_OPCODE_BREAKC:
        if (dst)
        {
            w.Line("if (({}.x) {} 0.f) {{ break; }}", EmitSrc(*dst),
                   instr.testNonZero ? "!=" : "==");
        }
        break;

    case D3D10_SB_OPCODE_SAMPLE:
    case D3D10_SB_OPCODE_SAMPLE_L:
        if (dst && src0 && src1 && src2)
        {
            auto uv = EmitSrc(*src0);
            EmitWrite(w, dstBase, mask,
                std::format("sw_sample_2d(res->tex[{}],res->smp[{}],({}).x,({}).y)",
                            src1->indices[0], src2->indices[0], uv, uv), sat);
        }
        break;

    case D3D10_SB_OPCODE_LD:
        if (dst && src0 && src1)
        {
            auto coord = EmitSrc(*src0);
            EmitWrite(w, dstBase, mask,
                std::format("sw_fetch_texel(res->tex[{}],(unsigned)(({}).x),(unsigned)(({}).y))",
                            src1->indices[0], coord, coord), sat);
        }
        break;

    case D3D11_SB_OPCODE_LD_UAV_TYPED:
        if (dst && src0 && src1)
        {
            EmitWrite(w, dstBase, mask,
                std::format("sw_uav_load_typed(res->uav[{}],sw_bits_uint(({}).x))",
                            src1->indices[0], EmitSrc(*src0)), sat);
        }
        break;

    case D3D11_SB_OPCODE_STORE_UAV_TYPED:
        if (dst && src0 && src1)
        {
            w.Line("sw_uav_store_typed(res->uav[{}],sw_bits_uint(({}).x),{});",
                   dst->indices[0], EmitSrc(*src0), EmitSrc(*src1));
        }
        break;

    case D3D11_SB_OPCODE_LD_RAW:
        if (dst && src0 && src1)
        {
            auto addr = EmitSrc(*src0);
            if (src1->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                EmitWrite(w, dstBase, mask,
                    std::format("sw_tgsm_load_raw(tgsm[{}],sw_bits_uint(({}).x))",
                                src1->indices[0], addr), sat);
            }
            else
            {
                EmitWrite(w, dstBase, mask,
                    std::format("sw_uav_load_raw(res->uav[{}],sw_bits_uint(({}).x))",
                                src1->indices[0], addr), sat);
            }
        }
        break;

    case D3D11_SB_OPCODE_STORE_RAW:
        if (dst && src0 && src1)
        {
            if (dst->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                w.Line("sw_tgsm_store_raw(tgsm[{}],sw_bits_uint(({}).x),{});",
                       dst->indices[0], EmitSrc(*src0), EmitSrc(*src1));
            }
            else
            {
                w.Line("sw_uav_store_raw(res->uav[{}],sw_bits_uint(({}).x),{});",
                       dst->indices[0], EmitSrc(*src0), EmitSrc(*src1));
            }
        }
        break;

    case D3D11_SB_OPCODE_LD_STRUCTURED:
        if (dst && src0 && src1 && src2)
        {
            auto idx = EmitSrc(*src0);
            auto offset = EmitSrc(*src1);
            if (src2->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                EmitWrite(w, dstBase, mask,
                    std::format("sw_tgsm_load_structured(tgsm[{}],sw_bits_uint(({}).x),sw_bits_uint(({}).x))",
                                src2->indices[0], idx, offset), sat);
            }
            else
            {
                EmitWrite(w, dstBase, mask,
                    std::format("sw_uav_load_structured(res->uav[{}],sw_bits_uint(({}).x),sw_bits_uint(({}).x))",
                                src2->indices[0], idx, offset), sat);
            }
        }
        break;

    case D3D11_SB_OPCODE_STORE_STRUCTURED:
        if (dst && src0 && src1 && src2)
        {
            if (dst->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                w.Line("sw_tgsm_store_structured(tgsm[{}],sw_bits_uint(({}).x),sw_bits_uint(({}).x),{});",
                       dst->indices[0], EmitSrc(*src0), EmitSrc(*src1), EmitSrc(*src2));
            }
            else
            {
                w.Line("sw_uav_store_structured(res->uav[{}],sw_bits_uint(({}).x),sw_bits_uint(({}).x),{});",
                       dst->indices[0], EmitSrc(*src0), EmitSrc(*src1), EmitSrc(*src2));
            }
        }
        break;

    case D3D10_SB_OPCODE_RET:
        w.Line("return;");
        break;

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
    case D3D10_SB_OPCODE_DCL_GLOBAL_FLAGS:
    case D3D11_SB_OPCODE_DCL_THREAD_GROUP:
    case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED:
    case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW:
    case D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED:
    case D3D11_SB_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW:
    case D3D11_SB_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED:
        break;

    case D3D11_SB_OPCODE_SYNC:
        w.Line("_barrier(_barrier_ctx);");
        break;

    default:
        w.Line("/* unhandled op={} */", (Int)instr.op);
        break;
    }
}

void EmitInstructions(CodeWriter& w, const D3D11SW_ParsedShader& shader)
{
    for (const auto& instr : shader.instrs)
    {
        EmitInstr(w, instr, shader);
    }
}

void EmitVS(CodeWriter& w, const D3D11SW_ParsedShader& shader)
{
    Uint32 numTemps = shader.numTemps > 0 ? shader.numTemps : 1;
    Uint8 svPosReg = FindSVPositionReg(shader);

    w.Line("extern \"C\" void ShaderMain(const SW_VSInput* in_ptr, SW_VSOutput* out_ptr,"
           " const SW_Resources* res)");
    w.Line("{{");
    w.Indent();
    w.Line("SW_float4 r[{}] = {{}};", numTemps);
    w.Line("SW_float4 out_v[{}] = {{}};", (Int)D3D11_VS_OUTPUT_REGISTER_COUNT);
    w.Newline();

    EmitInstructions(w, shader);

    w.Newline();
    for (const auto& e : shader.outputs)
    {
        if (e.svType == D3D_NAME_POSITION)
        {
            w.Line("out_ptr->pos = out_v[{}];", e.reg);
        }
        else
        {
            w.Line("out_ptr->o[{}] = out_v[{}];", e.reg, e.reg);
        }
    }

    if (svPosReg == 0xFF && !shader.outputs.empty())
    {
        w.Line("out_ptr->pos = out_v[0];");
    }

    w.Dedent();
    w.Line("}}");
}

void EmitPS(CodeWriter& w, const D3D11SW_ParsedShader& shader)
{
    Uint32 numTemps = shader.numTemps > 0 ? shader.numTemps : 1;

    w.Line("extern \"C\" void ShaderMain(const SW_PSInput* in_ptr, SW_PSOutput* out_ptr,"
           " const SW_Resources* res)");
    w.Line("{{");
    w.Indent();
    w.Line("SW_float4 r[{}] = {{}};", numTemps);
    w.Line("SW_float4 out_v[{}] = {{}};", (Int)D3D11_PS_OUTPUT_REGISTER_COUNT);
    w.Line("out_ptr->depthWritten = false;");
    w.Newline();

    EmitInstructions(w, shader);

    w.Newline();
    for (Uint32 i = 0; i < D3D11_PS_OUTPUT_REGISTER_COUNT; ++i)
    {
        w.Line("out_ptr->oC[{}] = out_v[{}];", i, i);
    }

    w.Dedent();
    w.Line("}}");
}

void EmitCS(CodeWriter& w, const D3D11SW_ParsedShader& shader)
{
    Uint32 numTemps = shader.numTemps > 0 ? shader.numTemps : 1;

    w.Line("extern \"C\" void ShaderMain(const SW_CSInput* in_ptr,"
           " SW_Resources* res, SW_TGSM* tgsm, SW_BarrierFn _barrier, void* _barrier_ctx)");
    w.Line("{{");
    w.Indent();
    w.Line("SW_float4 r[{}] = {{}};", numTemps);
    w.Line("SW_float4 _tid  = {{ sw_uint_bits(in_ptr->dispatchThreadID.x),"
           " sw_uint_bits(in_ptr->dispatchThreadID.y),"
           " sw_uint_bits(in_ptr->dispatchThreadID.z), 0.f }};");
    w.Line("SW_float4 _gid  = {{ sw_uint_bits(in_ptr->groupID.x),"
           " sw_uint_bits(in_ptr->groupID.y),"
           " sw_uint_bits(in_ptr->groupID.z), 0.f }};");
    w.Line("SW_float4 _gtid = {{ sw_uint_bits(in_ptr->groupThreadID.x),"
           " sw_uint_bits(in_ptr->groupThreadID.y),"
           " sw_uint_bits(in_ptr->groupThreadID.z), 0.f }};");
    w.Line("SW_float4 _gi   = {{ sw_uint_bits(in_ptr->groupIndex), 0.f, 0.f, 0.f }};");
    w.Line("(void)_tid; (void)_gid; (void)_gtid; (void)_gi; (void)tgsm;");
    w.Newline();

    EmitInstructions(w, shader);

    w.Dedent();
    w.Line("}}");
}

} 

std::string EmitShader(const D3D11SW_ParsedShader& shader,
                       const std::string& runtimeHeaderPath)
{
    CodeWriter w;
    w.Line("#include \"{}\"", runtimeHeaderPath);
    w.Newline();

    switch (shader.type)
    {
    case D3D11SW_ShaderType::Vertex:  EmitVS(w, shader); break;
    case D3D11SW_ShaderType::Pixel:   EmitPS(w, shader); break;
    case D3D11SW_ShaderType::Compute: EmitCS(w, shader); break;
    default:
        w.Line("extern \"C\" void ShaderMain() {{}}");
        break;
    }

    return w.Take();
}

}
