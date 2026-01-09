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
        if (op.relativeReg >= 0)
        {
            static const Char* compName[] = {"x","y","z","w"};
            base = std::format("res->cb[{}][{} + sw_bits_uint(r[{}].{})]",
                op.indices[0], op.indices[1], op.relativeReg, compName[op.relativeComp]);
        }
        else
        {
            base = std::format("res->cb[{}][{}]", op.indices[0], op.indices[1]);
        }
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
    if (dst == "out_ptr->oDepth")
    {
        w.Line("{{ SW_float4 _t = {};", expr);
        if (sat)
        {
            w.Line("  out_ptr->oDepth = sw_saturate(_t.x);");
        }
        else
        {
            w.Line("  out_ptr->oDepth = _t.x;");
        }
        w.Line("  out_ptr->depthWritten = true;");
        w.Line("}}");
        return;
    }
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

void EmitPerComp3(CodeWriter& w, const Char* fn,
                  std::string_view dst, Uint8 mask,
                  const SM4Operand& a, const SM4Operand& b, const SM4Operand& c)
{
    w.Line("{{ SW_float4 _a = {}, _b = {}, _c = {};", EmitSrc(a), EmitSrc(b), EmitSrc(c));
    for (Int i = 0; i < 4; ++i)
    {
        if (!(mask & (1 << i)))
        {
            continue;
        }
        w.Line("  {}.{} = {}(_a.{}, _b.{}, _c.{});", dst, Comp(i), fn, Comp(i), Comp(i), Comp(i));
    }
    w.Line("}}");
}

void EmitPerComp4(CodeWriter& w, const Char* fn,
                  std::string_view dst, Uint8 mask,
                  const SM4Operand& a, const SM4Operand& b,
                  const SM4Operand& c, const SM4Operand& d)
{
    w.Line("{{ SW_float4 _a = {}, _b = {}, _c = {}, _d = {};", EmitSrc(a), EmitSrc(b), EmitSrc(c), EmitSrc(d));
    for (Int i = 0; i < 4; ++i)
    {
        if (!(mask & (1 << i)))
        {
            continue;
        }
        w.Line("  {}.{} = {}(_a.{}, _b.{}, _c.{}, _d.{});", dst, Comp(i), fn, Comp(i), Comp(i), Comp(i), Comp(i));
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
    case D3D10_SB_OPCODE_UMUL: case D3D10_SB_OPCODE_UMAD:
    case D3D11_SB_OPCODE_UADDC: case D3D11_SB_OPCODE_USUBB:
    case D3D10_SB_OPCODE_ISHL: case D3D10_SB_OPCODE_ISHR: case D3D10_SB_OPCODE_USHR:
    case D3D10_SB_OPCODE_AND:  case D3D10_SB_OPCODE_OR:   case D3D10_SB_OPCODE_XOR:
    case D3D10_SB_OPCODE_NOT:  case D3D10_SB_OPCODE_INEG:
    case D3D10_SB_OPCODE_IMAX: case D3D10_SB_OPCODE_IMIN:
    case D3D10_SB_OPCODE_UMAX: case D3D10_SB_OPCODE_UMIN:
    case D3D10_SB_OPCODE_IEQ:  case D3D10_SB_OPCODE_INE:
    case D3D10_SB_OPCODE_IGE:  case D3D10_SB_OPCODE_ILT:
    case D3D10_SB_OPCODE_UGE:  case D3D10_SB_OPCODE_ULT:
    case D3D10_SB_OPCODE_UDIV:
    case D3D11_SB_OPCODE_COUNTBITS:
    case D3D11_SB_OPCODE_FIRSTBIT_HI: case D3D11_SB_OPCODE_FIRSTBIT_LO: case D3D11_SB_OPCODE_FIRSTBIT_SHI:
    case D3D11_SB_OPCODE_BFREV:
    case D3D11_SB_OPCODE_UBFE: case D3D11_SB_OPCODE_IBFE: case D3D11_SB_OPCODE_BFI:
    case D3D11_SB_OPCODE_F32TOF16: case D3D11_SB_OPCODE_F16TOF32:
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
    SM4Operand op4 = instr.operands.size() > 4 ? fixOp(&instr.operands[4]) : SM4Operand{};

    const SM4Operand* dst  = instr.operands.size() > 0 ? &op0 : nullptr;
    const SM4Operand* src0 = instr.operands.size() > 1 ? &op1 : nullptr;
    const SM4Operand* src1 = instr.operands.size() > 2 ? &op2 : nullptr;
    const SM4Operand* src2 = instr.operands.size() > 3 ? &op3 : nullptr;
    const SM4Operand* src3 = instr.operands.size() > 4 ? &op4 : nullptr;

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
    case D3D10_SB_OPCODE_ROUND_NI: if (src0) { EmitPerComp1(w, "sw_round_ni", dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_ROUND_PI: if (src0) { EmitPerComp1(w, "sw_round_pi", dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_FTOI:     if (src0) { EmitPerComp1(w, "sw_ftoi",     dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_ITOF:     if (src0) { EmitPerComp1(w, "sw_itof",     dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_UTOF:     if (src0) { EmitPerComp1(w, "sw_utof",     dstBase, mask, *src0); } break;
    case D3D10_SB_OPCODE_FTOU:     if (src0) { EmitPerComp1(w, "sw_ftou",     dstBase, mask, *src0); } break;
    case D3D11_SB_OPCODE_RCP:      if (src0) { EmitPerComp1(w, "sw_rcp",      dstBase, mask, *src0); } break;
    case D3D11_SB_OPCODE_F32TOF16: if (src0) { EmitPerComp1(w, "sw_f32tof16", dstBase, mask, *src0); } break;
    case D3D11_SB_OPCODE_F16TOF32: if (src0) { EmitPerComp1(w, "sw_f16tof32", dstBase, mask, *src0); } break;
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

    case D3D10_SB_OPCODE_IMAD:        if (src0 && src1 && src2) { EmitPerComp3(w, "sw_imad", dstBase, mask, *src0, *src1, *src2); } break;
    case D3D10_SB_OPCODE_UMAD:        if (src0 && src1 && src2) { EmitPerComp3(w, "sw_umad", dstBase, mask, *src0, *src1, *src2); } break;
    case D3D11_SB_OPCODE_COUNTBITS:   if (src0) { EmitPerComp1(w, "sw_countbits",   dstBase, mask, *src0); } break;
    case D3D11_SB_OPCODE_FIRSTBIT_HI: if (src0) { EmitPerComp1(w, "sw_firstbit_hi", dstBase, mask, *src0); } break;
    case D3D11_SB_OPCODE_FIRSTBIT_LO: if (src0) { EmitPerComp1(w, "sw_firstbit_lo", dstBase, mask, *src0); } break;
    case D3D11_SB_OPCODE_FIRSTBIT_SHI:if (src0) { EmitPerComp1(w, "sw_firstbit_shi",dstBase, mask, *src0); } break;
    case D3D11_SB_OPCODE_BFREV:       if (src0) { EmitPerComp1(w, "sw_bfrev",       dstBase, mask, *src0); } break;
    case D3D11_SB_OPCODE_UBFE:        if (src0 && src1 && src2) { EmitPerComp3(w, "sw_ubfe", dstBase, mask, *src0, *src1, *src2); } break;
    case D3D11_SB_OPCODE_IBFE:        if (src0 && src1 && src2) { EmitPerComp3(w, "sw_ibfe", dstBase, mask, *src0, *src1, *src2); } break;
    case D3D11_SB_OPCODE_BFI:         if (src0 && src1 && src2 && src3) { EmitPerComp4(w, "sw_bfi", dstBase, mask, *src0, *src1, *src2, *src3); } break;

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
                w.Line("  {}.{} = (sw_bits_uint(_c.{}) != 0u) ? _t.{} : _f.{};",
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

    case D3D10_SB_OPCODE_UMUL:
    {
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
                    w.Line("  {}.{} = sw_umul_hi(_a.{}, _b.{});", hBase, Comp(i), Comp(i), Comp(i));
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
                    w.Line("  {}.{} = sw_umul_lo(_a.{}, _b.{});", lBase, Comp(i), Comp(i), Comp(i));
                }
            }
            w.Line("}}");
        }
        break;
    }

    case D3D11_SB_OPCODE_UADDC:
    {
        const SM4Operand* dstVal   = dst;
        const SM4Operand* dstCarry = src0;
        const SM4Operand* srcA     = src1;
        const SM4Operand* srcB     = src2;
        if (srcA && srcB)
        {
            w.Line("{{ SW_float4 _a = {}, _b = {};", EmitSrc(*srcA), EmitSrc(*srcB));
            Uint8 vMask = dstVal  ? (dstVal->writeMask  ? dstVal->writeMask  : 0xF) : 0xF;
            Uint8 cMask = dstCarry ? (dstCarry->writeMask ? dstCarry->writeMask : 0xF) : 0xF;
            std::string vBase = dstVal   ? EmitDstBase(*dstVal)   : "r[0]";
            std::string cBase = dstCarry ? EmitDstBase(*dstCarry) : "r[0]";
            for (Int i = 0; i < 4; ++i)
            {
                if (!(vMask & (1 << i)) && !(cMask & (1 << i)))
                {
                    continue;
                }
                w.Line("  {{ float _carry;");
                w.Line("    float _r = sw_uaddc(_a.{}, _b.{}, _carry);", Comp(i), Comp(i));
                if (vMask & (1 << i))
                {
                    w.Line("    {}.{} = _r;", vBase, Comp(i));
                }
                if (cMask & (1 << i))
                {
                    w.Line("    {}.{} = _carry;", cBase, Comp(i));
                }
                w.Line("  }}");
            }
            w.Line("}}");
        }
        break;
    }

    case D3D11_SB_OPCODE_USUBB:
    {
        const SM4Operand* dstVal    = dst;
        const SM4Operand* dstBorrow = src0;
        const SM4Operand* srcA      = src1;
        const SM4Operand* srcB      = src2;
        if (srcA && srcB)
        {
            w.Line("{{ SW_float4 _a = {}, _b = {};", EmitSrc(*srcA), EmitSrc(*srcB));
            Uint8 vMask = dstVal    ? (dstVal->writeMask    ? dstVal->writeMask    : 0xF) : 0xF;
            Uint8 bMask = dstBorrow ? (dstBorrow->writeMask ? dstBorrow->writeMask : 0xF) : 0xF;
            std::string vBase = dstVal    ? EmitDstBase(*dstVal)    : "r[0]";
            std::string bBase = dstBorrow ? EmitDstBase(*dstBorrow) : "r[0]";
            for (Int i = 0; i < 4; ++i)
            {
                if (!(vMask & (1 << i)) && !(bMask & (1 << i)))
                {
                    continue;
                }
                w.Line("  {{ float _borrow;");
                w.Line("    float _r = sw_usubb(_a.{}, _b.{}, _borrow);", Comp(i), Comp(i));
                if (vMask & (1 << i))
                {
                    w.Line("    {}.{} = _r;", vBase, Comp(i));
                }
                if (bMask & (1 << i))
                {
                    w.Line("    {}.{} = _borrow;", bBase, Comp(i));
                }
                w.Line("  }}");
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
            w.Line("if (sw_bits_uint({}.x) {} 0u)", EmitSrc(*dst), instr.testNonZero ? "!=" : "==");
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
            w.Line("if (sw_bits_uint({}.x) {} 0u) {{ break; }}", EmitSrc(*dst),
                   instr.testNonZero ? "!=" : "==");
        }
        break;

    case D3D10_SB_OPCODE_CONTINUE:
        w.Line("continue;");
        break;

    case D3D10_SB_OPCODE_CONTINUEC:
        if (dst)
        {
            w.Line("if (sw_bits_uint({}.x) {} 0u) {{ continue; }}", EmitSrc(*dst),
                   instr.testNonZero ? "!=" : "==");
        }
        break;

    case D3D10_SB_OPCODE_RETC:
        if (dst)
        {
            w.Line("if (sw_bits_uint({}.x) {} 0u) {{ goto _sw_end; }}", EmitSrc(*dst),
                   instr.testNonZero ? "!=" : "==");
        }
        break;

    case D3D10_SB_OPCODE_SWITCH:
        if (dst)
        {
            w.Line("switch (sw_bits_uint({}.x))", EmitSrc(*dst));
            w.Line("{{");
            w.Indent();
        }
        break;

    case D3D10_SB_OPCODE_CASE:
        if (dst)
        {
            w.Dedent();
            Uint32 bits;
            std::memcpy(&bits, &dst->imm[0], 4);
            w.Line("case 0x{:08x}u:", bits);
            w.Indent();
        }
        break;

    case D3D10_SB_OPCODE_DEFAULT:
        w.Dedent();
        w.Line("default:");
        w.Indent();
        break;

    case D3D10_SB_OPCODE_ENDSWITCH:
        w.Dedent();
        w.Line("}}");
        break;

    case D3D10_SB_OPCODE_SAMPLE:
        if (dst && src0 && src1 && src2)
        {
            auto uv = EmitSrc(*src0);
            EmitWrite(w, dstBase, mask,
                std::format("sw_sample_2d(res->srv[{}],res->smp[{}],({}).x,({}).y)",
                            src1->indices[0], src2->indices[0], uv, uv), sat);
        }
        break;

    case D3D10_SB_OPCODE_SAMPLE_L:
    case D3D10_SB_OPCODE_SAMPLE_B:
    case D3D10_SB_OPCODE_SAMPLE_D:
        if (dst && src0 && src1 && src2)
        {
            auto uv = EmitSrc(*src0);
            EmitWrite(w, dstBase, mask,
                std::format("sw_sample_2d(res->srv[{}],res->smp[{}],({}).x,({}).y)",
                            src1->indices[0], src2->indices[0], uv, uv), sat);
        }
        break;

    case D3D10_SB_OPCODE_SAMPLE_C:
    case D3D10_SB_OPCODE_SAMPLE_C_LZ:
        if (dst && src0 && src1 && src2 && src3)
        {
            auto uv  = EmitSrc(*src0);
            auto ref = EmitSrc(*src3);
            EmitWrite(w, dstBase, mask,
                std::format("sw_sample_2d_cmp(res->srv[{}],res->smp[{}],({}).x,({}).y,({}).x)",
                            src1->indices[0], src2->indices[0], uv, uv, ref), sat);
        }
        break;

    case D3D10_1_SB_OPCODE_GATHER4:
        if (dst && src0 && src1 && src2)
        {
            auto uv   = EmitSrc(*src0);
            int  comp = src1->swizzle[0];
            EmitWrite(w, dstBase, mask,
                std::format("sw_gather_2d(res->srv[{}],res->smp[{}],({}).x,({}).y,{})",
                            src1->indices[0], src2->indices[0], uv, uv, comp), sat);
        }
        break;

    case D3D11_SB_OPCODE_GATHER4_C:
        if (dst && src0 && src1 && src2 && src3)
        {
            auto uv   = EmitSrc(*src0);
            auto ref  = EmitSrc(*src3);
            int  comp = src1->swizzle[0];
            EmitWrite(w, dstBase, mask,
                std::format("sw_gather_2d_cmp(res->srv[{}],res->smp[{}],({}).x,({}).y,({}).x,{})",
                            src1->indices[0], src2->indices[0], uv, uv, ref, comp), sat);
        }
        break;

    case D3D10_SB_OPCODE_LD:
        if (dst && src0 && src1)
        {
            auto coord = EmitSrc(*src0);
            EmitWrite(w, dstBase, mask,
                std::format("sw_fetch_texel_3d(res->srv[{}],sw_bits_uint(({}).x),sw_bits_uint(({}).y),sw_bits_uint(({}).z))",
                            src1->indices[0], coord, coord, coord), sat);
        }
        break;

    case D3D11_SB_OPCODE_LD_UAV_TYPED:
        if (dst && src0 && src1)
        {
            auto addr = EmitSrc(*src0);
            EmitWrite(w, dstBase, mask,
                std::format("sw_uav_load_typed(res->uav[{}],sw_bits_uint(({}).x),sw_bits_uint(({}).y))",
                            src1->indices[0], addr, addr), sat);
        }
        break;

    case D3D11_SB_OPCODE_STORE_UAV_TYPED:
        if (dst && src0 && src1)
        {
            auto addr = EmitSrc(*src0);
            w.Line("sw_uav_store_typed(res->uav[{}],sw_bits_uint(({}).x),sw_bits_uint(({}).y),{});",
                   dst->indices[0], addr, addr, EmitSrc(*src1));
        }
        break;

    case D3D11_SB_OPCODE_LD_RAW:
        if (dst && src0 && src1)
        {
            auto addr = EmitSrc(*src0);
            std::string ref, fn;
            if (src1->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                ref = std::format("tgsm[{}]", src1->indices[0]);
                fn  = "sw_tgsm_load_raw";
            }
            else if (src1->type == D3D10_SB_OPERAND_TYPE_RESOURCE)
            {
                ref = std::format("res->srv[{}]", src1->indices[0]);
                fn  = "sw_srv_load_raw";
            }
            else
            {
                ref = std::format("res->uav[{}]", src1->indices[0]);
                fn  = "sw_uav_load_raw";
            }
            w.Line("{{ unsigned _addr = sw_bits_uint(({}).x);", addr);
            Int off = 0;
            for (Int i = 0; i < 4; ++i)
            {
                if (!(mask & (1 << i)))
                {
                    continue;
                }
                w.Line("  {}.{} = {}({}, _addr + {}).x;", dstBase, Comp(i), fn, ref, off);
                off += 4;
            }
            w.Line("}}");
        }
        break;

    case D3D11_SB_OPCODE_STORE_RAW:
        if (dst && src0 && src1)
        {
            auto addr = EmitSrc(*src0);
            auto val = EmitSrc(*src1);
            Bool isTgsm = dst->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY;
            std::string ref = isTgsm
                ? std::format("tgsm[{}]", dst->indices[0])
                : std::format("res->uav[{}]", dst->indices[0]);
            std::string fn = isTgsm ? "sw_tgsm_store_raw" : "sw_uav_store_raw";
            w.Line("{{ unsigned _addr = sw_bits_uint(({}).x); SW_float4 _v = {};", addr, val);
            Int off = 0;
            for (Int i = 0; i < 4; ++i)
            {
                if (!(mask & (1 << i)))
                {
                    continue;
                }
                w.Line("  {}({}, _addr + {}, SW_float4{{_v.{},0,0,0}});", fn, ref, off, Comp(i));
                off += 4;
            }
            w.Line("}}");
        }
        break;

    case D3D11_SB_OPCODE_LD_STRUCTURED:
        if (dst && src0 && src1 && src2)
        {
            auto idx = EmitSrc(*src0);
            auto offset = EmitSrc(*src1);
            std::string ref, fn;
            if (src2->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                ref = std::format("tgsm[{}]", src2->indices[0]);
                fn  = "sw_tgsm_load_structured";
            }
            else if (src2->type == D3D10_SB_OPERAND_TYPE_RESOURCE)
            {
                ref = std::format("res->srv[{}]", src2->indices[0]);
                fn  = "sw_srv_load_structured";
            }
            else
            {
                ref = std::format("res->uav[{}]", src2->indices[0]);
                fn  = "sw_uav_load_structured";
            }
            w.Line("{{ unsigned _idx = sw_bits_uint(({}).x), _off = sw_bits_uint(({}).x);", idx, offset);
            Int off = 0;
            for (Int i = 0; i < 4; ++i)
            {
                if (!(mask & (1 << i)))
                {
                    continue;
                }
                w.Line("  {}.{} = {}({}, _idx, _off + {}).x;", dstBase, Comp(i), fn, ref, off);
                off += 4;
            }
            w.Line("}}");
        }
        break;

    case D3D11_SB_OPCODE_STORE_STRUCTURED:
        if (dst && src0 && src1 && src2)
        {
            auto idx = EmitSrc(*src0);
            auto offset = EmitSrc(*src1);
            auto val = EmitSrc(*src2);
            Bool isTgsm = dst->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY;
            std::string ref = isTgsm
                ? std::format("tgsm[{}]", dst->indices[0])
                : std::format("res->uav[{}]", dst->indices[0]);
            std::string fn = isTgsm ? "sw_tgsm_store_structured" : "sw_uav_store_structured";
            w.Line("{{ unsigned _idx = sw_bits_uint(({}).x), _off = sw_bits_uint(({}).x); SW_float4 _v = {};",
                   idx, offset, val);
            Int off = 0;
            for (Int i = 0; i < 4; ++i)
            {
                if (!(mask & (1 << i)))
                {
                    continue;
                }
                w.Line("  {}({}, _idx, _off + {}, SW_float4{{_v.{},0,0,0}});", fn, ref, off, Comp(i));
                off += 4;
            }
            w.Line("}}");
        }
        break;

    case D3D11_SB_OPCODE_ATOMIC_IADD:
    case D3D11_SB_OPCODE_ATOMIC_AND:
    case D3D11_SB_OPCODE_ATOMIC_OR:
    case D3D11_SB_OPCODE_ATOMIC_XOR:
    case D3D11_SB_OPCODE_ATOMIC_IMAX:
    case D3D11_SB_OPCODE_ATOMIC_IMIN:
    case D3D11_SB_OPCODE_ATOMIC_UMAX:
    case D3D11_SB_OPCODE_ATOMIC_UMIN:
        if (dst && src0 && src1)
        {
            const Char* fn = nullptr;
            switch (instr.op)
            {
            case D3D11_SB_OPCODE_ATOMIC_IADD: fn = "atomic_iadd"; break;
            case D3D11_SB_OPCODE_ATOMIC_AND:  fn = "atomic_and";  break;
            case D3D11_SB_OPCODE_ATOMIC_OR:   fn = "atomic_or";   break;
            case D3D11_SB_OPCODE_ATOMIC_XOR:  fn = "atomic_xor";  break;
            case D3D11_SB_OPCODE_ATOMIC_IMAX: fn = "atomic_imax"; break;
            case D3D11_SB_OPCODE_ATOMIC_IMIN: fn = "atomic_imin"; break;
            case D3D11_SB_OPCODE_ATOMIC_UMAX: fn = "atomic_umax"; break;
            case D3D11_SB_OPCODE_ATOMIC_UMIN: fn = "atomic_umin"; break;
            default: break;
            }
            if (dst->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                w.Line("sw_tgsm_{}(tgsm[{}],sw_bits_uint(({}).x),({}).x);",
                       fn, dst->indices[0], EmitSrc(*src0), EmitSrc(*src1));
            }
            else
            {
                w.Line("sw_uav_{}(res->uav[{}],sw_bits_uint(({}).x),({}).x);",
                       fn, dst->indices[0], EmitSrc(*src0), EmitSrc(*src1));
            }
        }
        break;

    case D3D11_SB_OPCODE_ATOMIC_CMP_STORE:
        if (dst && src0 && src1 && src2)
        {
            if (dst->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                w.Line("sw_tgsm_atomic_cmp_store(tgsm[{}],sw_bits_uint(({}).x),({}).x,({}).x);",
                       dst->indices[0], EmitSrc(*src0), EmitSrc(*src1), EmitSrc(*src2));
            }
            else
            {
                w.Line("sw_uav_atomic_cmp_store(res->uav[{}],sw_bits_uint(({}).x),({}).x,({}).x);",
                       dst->indices[0], EmitSrc(*src0), EmitSrc(*src1), EmitSrc(*src2));
            }
        }
        break;

    case D3D11_SB_OPCODE_IMM_ATOMIC_IADD:
    case D3D11_SB_OPCODE_IMM_ATOMIC_AND:
    case D3D11_SB_OPCODE_IMM_ATOMIC_OR:
    case D3D11_SB_OPCODE_IMM_ATOMIC_XOR:
    case D3D11_SB_OPCODE_IMM_ATOMIC_EXCH:
    case D3D11_SB_OPCODE_IMM_ATOMIC_IMAX:
    case D3D11_SB_OPCODE_IMM_ATOMIC_IMIN:
    case D3D11_SB_OPCODE_IMM_ATOMIC_UMAX:
    case D3D11_SB_OPCODE_IMM_ATOMIC_UMIN:
        if (dst && src0 && src1 && src2)
        {
            const Char* fn = nullptr;
            switch (instr.op)
            {
            case D3D11_SB_OPCODE_IMM_ATOMIC_IADD: fn = "imm_atomic_iadd"; break;
            case D3D11_SB_OPCODE_IMM_ATOMIC_AND:  fn = "imm_atomic_and";  break;
            case D3D11_SB_OPCODE_IMM_ATOMIC_OR:   fn = "imm_atomic_or";   break;
            case D3D11_SB_OPCODE_IMM_ATOMIC_XOR:  fn = "imm_atomic_xor";  break;
            case D3D11_SB_OPCODE_IMM_ATOMIC_EXCH: fn = "imm_atomic_exch"; break;
            case D3D11_SB_OPCODE_IMM_ATOMIC_IMAX: fn = "imm_atomic_imax"; break;
            case D3D11_SB_OPCODE_IMM_ATOMIC_IMIN: fn = "imm_atomic_imin"; break;
            case D3D11_SB_OPCODE_IMM_ATOMIC_UMAX: fn = "imm_atomic_umax"; break;
            case D3D11_SB_OPCODE_IMM_ATOMIC_UMIN: fn = "imm_atomic_umin"; break;
            default: break;
            }
            std::string retBase = EmitDstBase(*dst);
            if (src0->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                w.Line("{}.x = sw_tgsm_{}(tgsm[{}],sw_bits_uint(({}).x),({}).x);",
                       retBase, fn, src0->indices[0], EmitSrc(*src1), EmitSrc(*src2));
            }
            else
            {
                w.Line("{}.x = sw_uav_{}(res->uav[{}],sw_bits_uint(({}).x),({}).x);",
                       retBase, fn, src0->indices[0], EmitSrc(*src1), EmitSrc(*src2));
            }
        }
        break;

    case D3D11_SB_OPCODE_IMM_ATOMIC_CMP_EXCH:
        if (dst && src0 && src1 && src2 && src3)
        {
            std::string retBase = EmitDstBase(*dst);
            if (src0->type == D3D11_SB_OPERAND_TYPE_THREAD_GROUP_SHARED_MEMORY)
            {
                w.Line("{}.x = sw_tgsm_imm_atomic_cmp_exch(tgsm[{}],sw_bits_uint(({}).x),({}).x,({}).x);",
                       retBase, src0->indices[0], EmitSrc(*src1), EmitSrc(*src2), EmitSrc(*src3));
            }
            else
            {
                w.Line("{}.x = sw_uav_imm_atomic_cmp_exch(res->uav[{}],sw_bits_uint(({}).x),({}).x,({}).x);",
                       retBase, src0->indices[0], EmitSrc(*src1), EmitSrc(*src2), EmitSrc(*src3));
            }
        }
        break;

    case D3D10_SB_OPCODE_RET:
        w.Line("goto _sw_end;");
        break;

    case D3D10_SB_OPCODE_RESINFO:
        if (dst && src0 && src1)
        {
            const Char* fn = "sw_resinfo_float";
            if (instr.resInfoReturn == D3D10_SB_RESINFO_INSTRUCTION_RETURN_RCPFLOAT)
            {
                fn = "sw_resinfo_rcpfloat";
            }
            else if (instr.resInfoReturn == D3D10_SB_RESINFO_INSTRUCTION_RETURN_UINT)
            {
                fn = "sw_resinfo_uint";
            }
            EmitWrite(w, dstBase, mask,
                std::format("{}(res->srv[{}])", fn, src1->indices[0]), sat);
        }
        break;

    case D3D11_SB_OPCODE_BUFINFO:
        if (dst && src0)
        {
            EmitWrite(w, dstBase, mask,
                std::format("sw_bufinfo(res->uav[{}])", src0->indices[0]), sat);
        }
        break;


    case D3D10_SB_OPCODE_NOP:
    case D3D11_SB_OPCODE_GATHER4_PO:
    case D3D11_SB_OPCODE_GATHER4_PO_C:
        break;

    case D3D10_SB_OPCODE_DISCARD:
        if (dst)
        {
            w.Line("if (sw_bits_uint({}.x) {} 0u) {{ out_ptr->discarded = true; goto _sw_end; }}",
                   EmitSrc(*dst), instr.testNonZero ? "!=" : "==");
        }
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

    w.Line("extern \"C\" SW_JIT_EXPORT void ShaderMain(const SW_VSInput* in_ptr, SW_VSOutput* out_ptr,"
           " const SW_Resources* res)");
    w.Line("{{");
    w.Indent();
    w.Line("SW_float4 r[{}] = {{}};", numTemps);
    w.Line("SW_float4 out_v[{}] = {{}};", (Int)D3D11_VS_OUTPUT_REGISTER_COUNT);
    w.Newline();

    EmitInstructions(w, shader);

    w.Newline();
    w.Line("_sw_end:;");
    for (const auto& e : shader.outputs)
    {
        if (e.svType == D3D_NAME_POSITION)
        {
            w.Line("out_ptr->pos = out_v[{}];", e.reg);
        }
        else if (e.svType == D3D_NAME_CLIP_DISTANCE)
        {
            Int base = e.semanticIndex * 4;
            for (Int c = 0; c < 4; ++c)
            {
                if (e.mask & (1 << c))
                {
                    w.Line("out_ptr->clipDist[{}] = out_v[{}].{};", base + c, e.reg, Comp(c));
                }
            }
        }
        else if (e.svType == D3D_NAME_CULL_DISTANCE)
        {
            Int base = e.semanticIndex * 4;
            for (Int c = 0; c < 4; ++c)
            {
                if (e.mask & (1 << c))
                {
                    w.Line("out_ptr->cullDist[{}] = out_v[{}].{};", base + c, e.reg, Comp(c));
                }
            }
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

    w.Line("extern \"C\" SW_JIT_EXPORT void ShaderMain(const SW_PSInput* in_ptr, SW_PSOutput* out_ptr,"
           " const SW_Resources* res)");
    w.Line("{{");
    w.Indent();
    w.Line("SW_float4 r[{}] = {{}};", numTemps);
    w.Line("SW_float4 out_v[{}] = {{}};", (Int)D3D11_PS_OUTPUT_REGISTER_COUNT);
    w.Line("out_ptr->depthWritten = false;");
    w.Line("out_ptr->discarded = false;");
    w.Newline();

    for (const auto& e : shader.inputs)
    {
        if (e.svType == D3D_NAME_IS_FRONT_FACE)
        {
            w.Line("in_ptr->v[{}] = {{ sw_uint_bits(in_ptr->isFrontFace ? 0xFFFFFFFFu : 0u), 0.f, 0.f, 0.f }};", e.reg);
        }
    }

    EmitInstructions(w, shader);

    w.Newline();
    w.Line("_sw_end:;");
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

    w.Line("extern \"C\" SW_JIT_EXPORT void ShaderMain(const SW_CSInput* in_ptr,"
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

    w.Newline();
    w.Line("_sw_end:;");
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
        w.Line("extern \"C\" SW_JIT_EXPORT void ShaderMain() {{}}");
        break;
    }

    return w.Take();
}

}
