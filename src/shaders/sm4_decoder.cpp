#include "shaders/sm4_decoder.h"
#include <cstring>

namespace d3d11sw {

// OpcodeToken0
static constexpr Uint32 OT0_OPCODE_MASK          = 0x7FF;
static constexpr Uint32 OT0_SATURATE_SHIFT        = 13;
static constexpr Uint32 OT0_INSTR_LENGTH_SHIFT    = 24;
static constexpr Uint32 OT0_INSTR_LENGTH_MASK     = 0x7F;
static constexpr Uint32 OT0_EXTENDED_SHIFT        = 31;

// OperandToken0
static constexpr Uint32 OP0_NUM_COMP_MASK         = 0x3;
static constexpr Uint32 OP0_NUM_COMP_4            = 2;
static constexpr Uint32 OP0_NUM_COMP_1            = 1;
static constexpr Uint32 OP0_COMP_MODE_SHIFT       = 2;
static constexpr Uint32 OP0_COMP_MODE_MASK        = 0x3;
static constexpr Uint32 OP0_COMP_SEL_SHIFT        = 4;
static constexpr Uint32 OP0_COMP_SEL_MASK         = 0xFF;
static constexpr Uint32 OP0_OPERAND_TYPE_SHIFT    = 12;
static constexpr Uint32 OP0_OPERAND_TYPE_MASK     = 0xFF;
static constexpr Uint32 OP0_INDEX_DIM_SHIFT       = 20;
static constexpr Uint32 OP0_INDEX_DIM_MASK        = 0x3;
static constexpr Uint32 OP0_INDEX_REPR_BASE_SHIFT = 22;
static constexpr Uint32 OP0_INDEX_REPR_BITS       = 3;
static constexpr Uint32 OP0_INDEX_REPR_MASK       = 0x7;
static constexpr Uint32 OP0_EXTENDED_SHIFT        = 31;

// Extended operand token
static constexpr Uint32 EXT_TYPE_MASK             = 0x3F;
static constexpr Uint32 EXT_TYPE_MODIFIER         = 1;
static constexpr Uint32 EXT_MODIFIER_SHIFT        = 6;
static constexpr Uint32 EXT_MODIFIER_MASK         = 0xF;
static constexpr Uint32 EXT_MODIFIER_NEG          = 1;
static constexpr Uint32 EXT_MODIFIER_ABS          = 2;
static constexpr Uint32 EXT_MODIFIER_ABSNEG       = 3;

// Index representation values (D3D10_SB_OPERAND_INDEX_REPRESENTATION)
static constexpr Uint32 INDEX_REPR_IMM32          = 0;
static constexpr Uint32 INDEX_REPR_IMM64          = 1;
static constexpr Uint32 INDEX_REPR_RELATIVE       = 2;
static constexpr Uint32 INDEX_REPR_IMM32_RELATIVE = 3;
static constexpr Uint32 INDEX_REPR_IMM64_RELATIVE = 4;

// Component selection modes (D3D10_SB_OPERAND_4_COMPONENT_SELECTION_MODE)
static constexpr Uint32 COMP_MODE_MASK            = 0;
static constexpr Uint32 COMP_MODE_SWIZZLE         = 1;
static constexpr Uint32 COMP_MODE_SELECT1         = 2;

Uint32 SM4Decoder::ReadOperand(const Uint32* tokens, Uint32 offset, SM4Operand& op) const
{
    Uint32 t0       = tokens[offset];
    Uint32 consumed = 1;

    Uint32 numComp  = (t0                               ) & OP0_NUM_COMP_MASK;
    Uint32 compMode = (t0 >> OP0_COMP_MODE_SHIFT        ) & OP0_COMP_MODE_MASK;
    Uint32 compSel  = (t0 >> OP0_COMP_SEL_SHIFT         ) & OP0_COMP_SEL_MASK;
    Uint32 opType   = (t0 >> OP0_OPERAND_TYPE_SHIFT     ) & OP0_OPERAND_TYPE_MASK;
    Uint32 indexDim = (t0 >> OP0_INDEX_DIM_SHIFT        ) & OP0_INDEX_DIM_MASK;
    Bool   hasExt   = (t0 >> OP0_EXTENDED_SHIFT         ) & 1;

    if (hasExt)
    {
        Uint32 ext     = tokens[offset + consumed];
        consumed++;
        Uint32 extType = ext & EXT_TYPE_MASK;
        if (extType == EXT_TYPE_MODIFIER)
        {
            Uint32 mod  = (ext >> EXT_MODIFIER_SHIFT) & EXT_MODIFIER_MASK;
            op.negate   = (mod == EXT_MODIFIER_NEG   || mod == EXT_MODIFIER_ABSNEG);
            op.absolute = (mod == EXT_MODIFIER_ABS   || mod == EXT_MODIFIER_ABSNEG);
        }
    }

    op.type     = static_cast<SM4OperandType>(opType);
    op.indexDim = static_cast<Uint8>(indexDim);

    op.writeMask  = 0xF;
    op.swizzle[0] = 0; op.swizzle[1] = 1; op.swizzle[2] = 2; op.swizzle[3] = 3;

    if (numComp == OP0_NUM_COMP_4)
    {
        op.compMode = static_cast<SM4CompMode>(compMode);
        if (compMode == COMP_MODE_MASK)
        {
            op.writeMask = static_cast<Uint8>(compSel & 0xF);
        }
        else if (compMode == COMP_MODE_SWIZZLE)
        {
            op.swizzle[0] = static_cast<Uint8>((compSel     ) & 0x3);
            op.swizzle[1] = static_cast<Uint8>((compSel >> 2) & 0x3);
            op.swizzle[2] = static_cast<Uint8>((compSel >> 4) & 0x3);
            op.swizzle[3] = static_cast<Uint8>((compSel >> 6) & 0x3);
        }
        else if (compMode == COMP_MODE_SELECT1)
        {
            Uint8 sel     = static_cast<Uint8>(compSel & 0x3);
            op.swizzle[0] = op.swizzle[1] = op.swizzle[2] = op.swizzle[3] = sel;
        }
    }

    for (Uint32 dim = 0; dim < indexDim; ++dim)
    {
        Uint32 repr = (t0 >> (OP0_INDEX_REPR_BASE_SHIFT + OP0_INDEX_REPR_BITS * dim)) & OP0_INDEX_REPR_MASK;
        switch (repr)
        {
            case INDEX_REPR_IMM32:
            {
                op.indices[dim] = tokens[offset + consumed];
                consumed++;
                break;
            }
            case INDEX_REPR_IMM64:
            {
                op.indices[dim] = tokens[offset + consumed];
                consumed += 2;
                break;
            }
            case INDEX_REPR_RELATIVE:
            {
                SM4Operand dummy{};
                consumed += ReadOperand(tokens, offset + consumed, dummy);
                break;
            }
            case INDEX_REPR_IMM32_RELATIVE:
            {
                op.indices[dim] = tokens[offset + consumed];
                consumed++;
                SM4Operand dummy{};
                consumed += ReadOperand(tokens, offset + consumed, dummy);
                break;
            }
            case INDEX_REPR_IMM64_RELATIVE:
            {
                op.indices[dim] = tokens[offset + consumed];
                consumed += 2;
                SM4Operand dummy{};
                consumed += ReadOperand(tokens, offset + consumed, dummy);
                break;
            }
        }
    }

    if (op.type == SM4OperandType::Immediate32)
    {
        Uint32 count = (numComp == OP0_NUM_COMP_1) ? 1 : 4;
        for (Uint32 i = 0; i < count; ++i)
        {
            std::memcpy(&op.imm[i], &tokens[offset + consumed], 4);
            consumed++;
        }
    }

    return consumed;
}

Bool SM4Decoder::Decode(const Uint32* tokens, Uint32 numDwords,
                         std::vector<SM4Instruction>& out,
                         Uint32& numTempsOut,
                         Uint32  threadGroupSizeOut[3])
{
    numTempsOut = 0;
    threadGroupSizeOut[0] = threadGroupSizeOut[1] = threadGroupSizeOut[2] = 1;

    if (numDwords < 2)
    {
        return false;
    }

    Uint32 totalLen = tokens[1];
    if (totalLen > numDwords)
    {
        totalLen = numDwords;
    }

    Uint32 pos = 2;

    while (pos < totalLen)
    {
        Uint32    opTok    = tokens[pos];
        SM4OpCode op       = static_cast<SM4OpCode>(opTok & OT0_OPCODE_MASK);
        Bool      sat      = (opTok >> OT0_SATURATE_SHIFT) & 1;
        Uint32    instrLen = (opTok >> OT0_INSTR_LENGTH_SHIFT) & OT0_INSTR_LENGTH_MASK;

        if (instrLen == 0)
        {
            break;
        }

        Uint32 instrEnd = pos + instrLen;

        SM4Instruction instr{};
        instr.op       = op;
        instr.saturate = sat;

        if (op == SM4OpCode::DclTemps)
        {
            if (pos + 1 < instrEnd)
            {
                numTempsOut = tokens[pos + 1];
            }
            pos = instrEnd;
            out.push_back(std::move(instr));
            continue;
        }

        if (op == SM4OpCode::DclThreadGroup)
        {
            if (pos + 3 < instrEnd)
            {
                threadGroupSizeOut[0] = instr.threadGroupSize[0] = tokens[pos + 1];
                threadGroupSizeOut[1] = instr.threadGroupSize[1] = tokens[pos + 2];
                threadGroupSizeOut[2] = instr.threadGroupSize[2] = tokens[pos + 3];
            }
            pos = instrEnd;
            out.push_back(std::move(instr));
            continue;
        }

        if (op == SM4OpCode::DclResource                ||
            op == SM4OpCode::DclConstantBuffer          ||
            op == SM4OpCode::DclSampler                 ||
            op == SM4OpCode::DclIndexRange              ||
            op == SM4OpCode::DclGlobalFlags             ||
            op == SM4OpCode::DclIndexableTemp           ||
            op == SM4OpCode::DclInputSgv                ||
            op == SM4OpCode::DclInputSiv                ||
            op == SM4OpCode::DclInputPsSgv              ||
            op == SM4OpCode::DclInputPsSiv              ||
            op == SM4OpCode::DclOutputSgv               ||
            op == SM4OpCode::DclOutputSiv               ||
            op == SM4OpCode::DclGsOutputPrimitiveTopo   ||
            op == SM4OpCode::DclGsInputPrimitive        ||
            op == SM4OpCode::DclMaxOutputVertexCount)
        {
            pos = instrEnd;
            out.push_back(std::move(instr));
            continue;
        }

        Uint32 cur = pos + 1;
        if ((opTok >> OT0_EXTENDED_SHIFT) & 1)
        {
            cur++;
        }

        while (cur < instrEnd)
        {
            SM4Operand operand{};
            Uint32 eaten = ReadOperand(tokens, cur, operand);
            instr.operands.push_back(operand);
            cur += eaten;
        }

        pos = instrEnd;
        out.push_back(std::move(instr));
    }

    return true;
}

} 
