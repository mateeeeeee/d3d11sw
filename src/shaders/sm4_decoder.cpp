#include "shaders/sm4_decoder.h"
#include "common/log.h"
#include <cstring>

namespace d3d11sw {

static Uint32 ReadOperand(const Uint32* tokens, Uint32 offset, SM4Operand& op)
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

    op.type     = static_cast<D3D10_SB_OPERAND_TYPE>(opType);
    op.indexDim = static_cast<Uint8>(indexDim);

    op.writeMask  = 0xF;
    op.swizzle[0] = 0; op.swizzle[1] = 1; op.swizzle[2] = 2; op.swizzle[3] = 3;

    if (numComp == D3D10_SB_OPERAND_4_COMPONENT)
    {
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
                SM4Operand relOp{};
                consumed += ReadOperand(tokens, offset + consumed, relOp);
                if (relOp.type == D3D10_SB_OPERAND_TYPE_TEMP)
                {
                    op.relativeReg  = static_cast<Int>(relOp.indices[0]);
                    op.relativeComp = relOp.swizzle[0];
                }
                break;
            }
            case D3D10_SB_OPERAND_INDEX_IMMEDIATE32_PLUS_RELATIVE:
            {
                op.indices[dim] = tokens[offset + consumed];
                consumed++;
                SM4Operand relOp{};
                consumed += ReadOperand(tokens, offset + consumed, relOp);
                if (relOp.type == D3D10_SB_OPERAND_TYPE_TEMP)
                {
                    op.relativeReg  = static_cast<Int>(relOp.indices[0]);
                    op.relativeComp = relOp.swizzle[0];
                }
                break;
            }
            case D3D10_SB_OPERAND_INDEX_IMMEDIATE64_PLUS_RELATIVE:
            {
                op.indices[dim] = tokens[offset + consumed];
                consumed += 2;
                SM4Operand relOp{};
                consumed += ReadOperand(tokens, offset + consumed, relOp);
                if (relOp.type == D3D10_SB_OPERAND_TYPE_TEMP)
                {
                    op.relativeReg  = static_cast<Int>(relOp.indices[0]);
                    op.relativeComp = relOp.swizzle[0];
                }
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

Bool SM4Decode(const Uint32* tokens, Uint32 numDwords, D3D11SW_ParsedShader& out)
{
    out.numTemps = 0;
    out.threadGroupX = 1;
    out.threadGroupY = 1;
    out.threadGroupZ = 1;
    out.gsInputPrimitive = 0;
    out.gsOutputTopology = 0;
    out.gsMaxOutputVertices = 0;
    out.gsInstanceCount = 1;
    if (numDwords < 2)
    {
        D3D11SW_ERROR("SM4Decode: bytecode too short ({} dwords)", numDwords);
        return false;
    }

    Uint32 totalLen = tokens[1];
    if (totalLen > numDwords)
    {
        totalLen = numDwords;
    }

    enum class HSPhaseState { None, Decls, ControlPoint, Fork, Join };
    HSPhaseState hsPhase = HSPhaseState::None;
    std::vector<SM4Instruction>*        activeInstrs    = &out.instrs;
    Uint32*                             activeNumTemps  = &out.numTemps;
    std::vector<D3D11SW_IndexableTempDecl>* activeIdxTemps = &out.indexableTemps;
    auto switchPhase = [&](HSPhaseState newPhase)
    {
        hsPhase = newPhase;
        switch (newPhase)
        {
        case HSPhaseState::Decls:
            activeInstrs   = &out.instrs;
            activeNumTemps = &out.numTemps;
            activeIdxTemps = &out.indexableTemps;
            break;
        case HSPhaseState::ControlPoint:
            activeInstrs   = &out.hsControlPointPhase.instrs;
            activeNumTemps = &out.hsControlPointPhase.numTemps;
            activeIdxTemps = &out.hsControlPointPhase.indexableTemps;
            break;
        case HSPhaseState::Fork:
            out.hsForkPhases.emplace_back();
            activeInstrs   = &out.hsForkPhases.back().instrs;
            activeNumTemps = &out.hsForkPhases.back().numTemps;
            activeIdxTemps = &out.hsForkPhases.back().indexableTemps;
            break;
        case HSPhaseState::Join:
            out.hsJoinPhases.emplace_back();
            activeInstrs   = &out.hsJoinPhases.back().instrs;
            activeNumTemps = &out.hsJoinPhases.back().numTemps;
            activeIdxTemps = &out.hsJoinPhases.back().indexableTemps;
            break;
        default:
            break;
        }
    };

    Uint32 pos = 2;
    while (pos < totalLen)
    {
        Uint32    opTok    = tokens[pos];
        D3D10_SB_OPCODE_TYPE op       = DECODE_D3D10_SB_OPCODE_TYPE(opTok);
        Bool      sat      = DECODE_IS_D3D10_SB_INSTRUCTION_SATURATE_ENABLED(opTok) != 0;
        Uint32    instrLen = DECODE_D3D10_SB_TOKENIZED_INSTRUCTION_LENGTH(opTok);
        if (instrLen == 0)
        {
            break;
        }

        Uint32 instrEnd = pos + instrLen;

        if (op == D3D11_SB_OPCODE_HS_DECLS)
        {
            switchPhase(HSPhaseState::Decls);
            pos = instrEnd;
            continue;
        }
        if (op == D3D11_SB_OPCODE_HS_CONTROL_POINT_PHASE)
        {
            switchPhase(HSPhaseState::ControlPoint);
            pos = instrEnd;
            continue;
        }
        if (op == D3D11_SB_OPCODE_HS_FORK_PHASE)
        {
            switchPhase(HSPhaseState::Fork);
            pos = instrEnd;
            continue;
        }
        if (op == D3D11_SB_OPCODE_HS_JOIN_PHASE)
        {
            switchPhase(HSPhaseState::Join);
            pos = instrEnd;
            continue;
        }

        if (op == D3D10_SB_OPCODE_DCL_TEMPS)
        {
            if (pos + 1 < instrEnd)
            {
                *activeNumTemps = tokens[pos + 1];
            }
            pos = instrEnd;
            continue;
        }

        if (op == D3D10_SB_OPCODE_DCL_INDEXABLE_TEMP)
        {
            if (pos + 3 < instrEnd)
            {
                D3D11SW_IndexableTempDecl decl{};
                decl.regIndex = tokens[pos + 1];
                decl.numRegs  = tokens[pos + 2];
                decl.numComps = tokens[pos + 3];
                activeIdxTemps->push_back(decl);
            }
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_THREAD_GROUP)
        {
            if (pos + 3 < instrEnd)
            {
                out.threadGroupX = tokens[pos + 1];
                out.threadGroupY = tokens[pos + 2];
                out.threadGroupZ = tokens[pos + 3];
            }
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_RAW)
        {
            Uint32 cur = pos + 1;
            SM4Operand gOp{};
            cur += ReadOperand(tokens, cur, gOp);
            Uint32 elementCount = (cur < instrEnd) ? tokens[cur] : 0;
            D3D11SW_TGSMDecl decl{};
            decl.slot   = gOp.indices[0];
            decl.size   = elementCount * 4;
            decl.stride = 0;
            out.tgsm.push_back(decl);
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_THREAD_GROUP_SHARED_MEMORY_STRUCTURED)
        {
            Uint32 cur = pos + 1;
            SM4Operand gOp{};
            cur += ReadOperand(tokens, cur, gOp);
            Uint32 stride = (cur < instrEnd) ? tokens[cur] : 0;
            cur++;
            Uint32 count  = (cur < instrEnd) ? tokens[cur] : 0;
            D3D11SW_TGSMDecl decl{};
            decl.slot   = gOp.indices[0];
            decl.size   = stride * count;
            decl.stride = stride;
            out.tgsm.push_back(decl);
            pos = instrEnd;
            continue;
        }

        if (op == D3D10_SB_OPCODE_DCL_GS_INPUT_PRIMITIVE)
        {
            out.gsInputPrimitive = DECODE_D3D10_SB_GS_INPUT_PRIMITIVE(opTok);
            pos = instrEnd;
            continue;
        }

        if (op == D3D10_SB_OPCODE_DCL_GS_OUTPUT_PRIMITIVE_TOPOLOGY)
        {
            out.gsOutputTopology = DECODE_D3D10_SB_GS_OUTPUT_PRIMITIVE_TOPOLOGY(opTok);
            pos = instrEnd;
            continue;
        }

        if (op == D3D10_SB_OPCODE_DCL_MAX_OUTPUT_VERTEX_COUNT)
        {
            if (pos + 1 < instrEnd)
            {
                out.gsMaxOutputVertices = tokens[pos + 1];
            }
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_GS_INSTANCE_COUNT)
        {
            out.gsInstanceCount = tokens[pos + 1];
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_INPUT_CONTROL_POINT_COUNT)
        {
            out.hsInputControlPointCount = DECODE_D3D11_SB_INPUT_CONTROL_POINT_COUNT(opTok);
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_OUTPUT_CONTROL_POINT_COUNT)
        {
            out.hsOutputControlPointCount = DECODE_D3D11_SB_OUTPUT_CONTROL_POINT_COUNT(opTok);
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_TESS_DOMAIN)
        {
            out.hsDomain = DECODE_D3D11_SB_TESS_DOMAIN(opTok);
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_TESS_PARTITIONING)
        {
            out.hsPartitioning = DECODE_D3D11_SB_TESS_PARTITIONING(opTok);
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_TESS_OUTPUT_PRIMITIVE)
        {
            out.hsOutputPrimitive = DECODE_D3D11_SB_TESS_OUTPUT_PRIMITIVE(opTok);
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_HS_MAX_TESSFACTOR)
        {
            if (pos + 1 < instrEnd)
            {
                std::memcpy(&out.hsMaxTessFactor, &tokens[pos + 1], 4);
            }
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_HS_FORK_PHASE_INSTANCE_COUNT)
        {
            if (!out.hsForkPhases.empty() && pos + 1 < instrEnd)
            {
                out.hsForkPhases.back().instanceCount = tokens[pos + 1];
            }
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_HS_JOIN_PHASE_INSTANCE_COUNT)
        {
            if (!out.hsJoinPhases.empty() && pos + 1 < instrEnd)
            {
                out.hsJoinPhases.back().instanceCount = tokens[pos + 1];
            }
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_STREAM ||
            op == D3D11_SB_OPCODE_DCL_FUNCTION_BODY ||
            op == D3D11_SB_OPCODE_DCL_FUNCTION_TABLE ||
            op == D3D11_SB_OPCODE_DCL_INTERFACE)
        {
            pos = instrEnd;
            continue;
        }

        if (op >= D3D10_SB_OPCODE_DCL_RESOURCE && op <= D3D10_SB_OPCODE_DCL_GLOBAL_FLAGS)
        {
            pos = instrEnd;
            continue;
        }

        if (op == D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_TYPED    ||
            op == D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_RAW      ||
            op == D3D11_SB_OPCODE_DCL_UNORDERED_ACCESS_VIEW_STRUCTURED)
        {
            pos = instrEnd;
            continue;
        }

        SM4Instruction instr{};
        instr.op          = op;
        instr.saturate    = sat;
        instr.testNonZero = DECODE_D3D10_SB_INSTRUCTION_TEST_BOOLEAN(opTok) == D3D10_SB_INSTRUCTION_TEST_NONZERO;
        if (op == D3D10_SB_OPCODE_RESINFO)
        {
            instr.resInfoReturn = static_cast<Uint8>(DECODE_D3D10_SB_RESINFO_INSTRUCTION_RETURN_TYPE(opTok));
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
        activeInstrs->push_back(std::move(instr));
    }

    return true;
}

}
