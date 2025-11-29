#include "shaders/sm4_decoder.h"
#include <cstring>

namespace d3d11sw {

Uint32 SM4Decoder::ReadOperand(const Uint32* tokens, Uint32 offset, SM4Operand& op) const
{
    Uint32 t0       = tokens[offset];
    Uint32 consumed = 1;

    Uint32 numComp  = DECODE_D3D10_SB_OPERAND_NUM_COMPONENTS(t0);
    Uint32 compMode = DECODE_D3D10_SB_OPERAND_4_COMPONENT_SELECTION_MODE(t0);
    Uint32 compSel  = (t0 >> 4) & 0xFF;
    Uint32 opType   = DECODE_D3D10_SB_OPERAND_TYPE(t0);
    Uint32 indexDim = DECODE_D3D10_SB_OPERAND_INDEX_DIMENSION(t0);
    Bool   hasExt   = DECODE_IS_D3D10_SB_OPERAND_EXTENDED(t0);

    if (hasExt)
    {
        Uint32 ext = tokens[offset + consumed];
        consumed++;
        if (DECODE_D3D10_SB_EXTENDED_OPERAND_TYPE(ext) == D3D10_SB_EXTENDED_OPERAND_MODIFIER)
        {
            D3D10_SB_OPERAND_MODIFIER mod = DECODE_D3D10_SB_OPERAND_MODIFIER(ext);
            op.negate   = (mod == D3D10_SB_OPERAND_MODIFIER_NEG    || mod == D3D10_SB_OPERAND_MODIFIER_ABSNEG);
            op.absolute = (mod == D3D10_SB_OPERAND_MODIFIER_ABS    || mod == D3D10_SB_OPERAND_MODIFIER_ABSNEG);
        }
    }

    op.type     = static_cast<SM4OperandType>(opType);
    op.indexDim = static_cast<Uint8>(indexDim);

    op.writeMask  = 0xF;
    op.swizzle[0] = 0; op.swizzle[1] = 1; op.swizzle[2] = 2; op.swizzle[3] = 3;

    if (numComp == D3D10_SB_OPERAND_4_COMPONENT)
    {
        op.compMode = static_cast<SM4CompMode>(compMode);
        if (compMode == D3D10_SB_OPERAND_4_COMPONENT_MASK_MODE)
        {
            op.writeMask = static_cast<Uint8>(compSel & 0xF);
        }
        else if (compMode == D3D10_SB_OPERAND_4_COMPONENT_SWIZZLE_MODE)
        {
            op.swizzle[0] = static_cast<Uint8>((compSel     ) & 0x3);
            op.swizzle[1] = static_cast<Uint8>((compSel >> 2) & 0x3);
            op.swizzle[2] = static_cast<Uint8>((compSel >> 4) & 0x3);
            op.swizzle[3] = static_cast<Uint8>((compSel >> 6) & 0x3);
        }
        else if (compMode == D3D10_SB_OPERAND_4_COMPONENT_SELECT_1_MODE)
        {
            Uint8 sel     = static_cast<Uint8>(compSel & 0x3);
            op.swizzle[0] = op.swizzle[1] = op.swizzle[2] = op.swizzle[3] = sel;
        }
    }

    for (Uint32 dim = 0; dim < indexDim; ++dim)
    {
        Uint32 repr = DECODE_D3D10_SB_OPERAND_INDEX_REPRESENTATION(dim, t0);
        switch (repr)
        {
            case D3D10_SB_OPERAND_INDEX_IMMEDIATE32:
            {
                op.indices[dim] = tokens[offset + consumed];
                consumed++;
                break;
            }
            case D3D10_SB_OPERAND_INDEX_IMMEDIATE64:
            {
                op.indices[dim] = tokens[offset + consumed];
                consumed += 2;
                break;
            }
            case D3D10_SB_OPERAND_INDEX_RELATIVE:
            {
                SM4Operand dummy{};
                consumed += ReadOperand(tokens, offset + consumed, dummy);
                break;
            }
            case D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
            {
                op.indices[dim] = tokens[offset + consumed];
                consumed++;
                SM4Operand dummy{};
                consumed += ReadOperand(tokens, offset + consumed, dummy);
                break;
            }
            case D3D10_SB_OPERAND_INDEX_IMMEDIATE64_PLUS_RELATIVE:
            {
                op.indices[dim] = tokens[offset + consumed];
                consumed += 2;
                SM4Operand dummy{};
                consumed += ReadOperand(tokens, offset + consumed, dummy);
                break;
            }
        }
    }

    if (op.type == D3D10_SB_OPERAND_TYPE_IMMEDIATE32)
    {
        Uint32 count = (numComp == D3D10_SB_OPERAND_1_COMPONENT) ? 1 : 4;
        for (Uint32 i = 0; i < count; ++i)
        {
            std::memcpy(&op.imm[i], &tokens[offset + consumed], 4);
            consumed++;
        }
        if (count == 1)
        {
            op.imm[1] = op.imm[2] = op.imm[3] = op.imm[0];
        }
    }

    return consumed;
}

Bool SM4Decoder::Decode(const Uint32* tokens, Uint32 numDwords,
                         std::vector<SM4Instruction>& out,
                         Uint32& numTempsOut,
                         Uint32  threadGroupSizeOut[3],
                         std::vector<D3D11SW_TGSMDecl>& tgsmOut)
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
        SM4OpCode op       = DECODE_D3D10_SB_OPCODE_TYPE(opTok);
        Bool      sat      = DECODE_IS_D3D10_SB_INSTRUCTION_SATURATE_ENABLED(opTok) != 0;
        Uint32    instrLen = DECODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(opTok);

        if (instrLen == 0)
        {
            break;
        }

        Uint32 instrEnd = pos + instrLen;

        SM4Instruction instr{};
        instr.op          = op;
        instr.saturate    = sat;
        instr.testNonZero = DECODE_D3D10_SB_INSTRUCTION_TEST_BOOLEAN(opTok) == D3D10_SB_INSTRUCTION_TEST_NONZERO;

        if (op == D3D10_SB_OPCODE_DCL_TEMPS)
        {
            if (pos + 1 < instrEnd)
            {
                numTempsOut = tokens[pos + 1];
            }
            pos = instrEnd;
            out.push_back(std::move(instr));
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_THREAD_GROUP)
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

        if (op == D3D11_SB_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW)
        {
            //Format: opcode, operand(g#), element_count (# of 32-bit scalars)
            Uint32 cur = pos + 1;
            SM4Operand gOp{};
            cur += ReadOperand(tokens, cur, gOp);
            Uint32 elementCount = (cur < instrEnd) ? tokens[cur] : 0;
            instr.tgsmSlot   = gOp.indices[0];
            instr.tgsmSize   = elementCount * 4; 
            instr.tgsmStride = 0; // raw
            D3D11SW_TGSMDecl decl{};
            decl.slot   = instr.tgsmSlot;
            decl.size   = instr.tgsmSize;
            decl.stride = 0;
            tgsmOut.push_back(decl);
            pos = instrEnd;
            out.push_back(std::move(instr));
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED)
        {
            //Format: opcode, operand(g#), struct_byte_stride, struct_count
            Uint32 cur = pos + 1;
            SM4Operand gOp{};
            cur += ReadOperand(tokens, cur, gOp);
            Uint32 stride = (cur < instrEnd) ? tokens[cur] : 0;
            cur++;
            Uint32 count  = (cur < instrEnd) ? tokens[cur] : 0;
            instr.tgsmSlot   = gOp.indices[0];
            instr.tgsmStride = stride;
            instr.tgsmSize   = stride * count;
            D3D11SW_TGSMDecl decl{};
            decl.slot   = instr.tgsmSlot;
            decl.size   = instr.tgsmSize;
            decl.stride = stride;
            tgsmOut.push_back(decl);
            pos = instrEnd;
            out.push_back(std::move(instr));
            continue;
        }

        if (op == D3D11_SB_OPCODE_SYNC)
        {
            pos = instrEnd;
            out.push_back(std::move(instr));
            continue;
        }

        if (op == D3D10_SB_OPCODE_DCL_RESOURCE                          ||
            op == D3D10_SB_OPCODE_DCL_CONSTANT_BUFFER                   ||
            op == D3D10_SB_OPCODE_DCL_SAMPLER                           ||
            op == D3D10_SB_OPCODE_DCL_INDEX_RANGE                       ||
            op == D3D10_SB_OPCODE_DCL_GLOBAL_FLAGS                      ||
            op == D3D10_SB_OPCODE_DCL_INDEXABLE_TEMP                    ||
            op == D3D10_SB_OPCODE_DCL_INPUT_SGV                         ||
            op == D3D10_SB_OPCODE_DCL_INPUT_SIV                         ||
            op == D3D10_SB_OPCODE_DCL_INPUT_PS_SGV                      ||
            op == D3D10_SB_OPCODE_DCL_INPUT_PS_SIV                      ||
            op == D3D10_SB_OPCODE_DCL_OUTPUT_SGV                        ||
            op == D3D10_SB_OPCODE_DCL_OUTPUT_SIV                        ||
            op == D3D10_SB_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY      ||
            op == D3D10_SB_OPCODE_DCL_GS_INPUT_PRIMITIVE                ||
            op == D3D10_SB_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT)
        {
            pos = instrEnd;
            out.push_back(std::move(instr));
            continue;
        }

        Uint32 cur = pos + 1;
        if (DECODE_IS_D3D10_SB_OPCODE_EXTENDED(opTok))
        {
            Uint32 ext = tokens[cur];
            cur++;
            while (ext >> 31)
            {
                ext = tokens[cur];
                cur++;
            }
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

} // namespace d3d11sw
