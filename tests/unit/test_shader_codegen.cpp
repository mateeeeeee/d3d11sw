#include <gtest/gtest.h>
#include "shaders/shader_codegen.h"
#include "shaders/dxbc_parser.h"
#include <d3d11TokenizedProgramFormat.hpp>

using namespace d3d11sw;

static SM4Instruction MakeInstr(SM4OpCode op,
                                  std::vector<SM4Operand> operands = {},
                                  Bool sat = false)
{
    SM4Instruction i{};
    i.op       = op;
    i.saturate = sat;
    i.operands = std::move(operands);
    return i;
}

static SM4Operand MakeTemp(Uint32 reg, Uint8 mask = 0xF)
{
    SM4Operand op{};
    op.type      = D3D10_SB_OPERAND_TYPE_TEMP;
    op.compMode  = D3D10_SB_OPERAND_4_COMPONENT_MASK_MODE;
    op.writeMask = mask;
    op.indices[0] = reg;
    op.indexDim   = 1;
    return op;
}

static SM4Operand MakeInput(Uint32 reg)
{
    SM4Operand op{};
    op.type      = D3D10_SB_OPERAND_TYPE_INPUT;
    op.compMode  = D3D10_SB_OPERAND_4_COMPONENT_SWIZZLE_MODE;
    op.swizzle[0] = 0; op.swizzle[1] = 1; op.swizzle[2] = 2; op.swizzle[3] = 3;
    op.indices[0] = reg;
    op.indexDim   = 1;
    return op;
}

static SM4Operand MakeOutput(Uint32 reg, Uint8 mask = 0xF)
{
    SM4Operand op{};
    op.type      = D3D10_SB_OPERAND_TYPE_OUTPUT;
    op.compMode  = D3D10_SB_OPERAND_4_COMPONENT_MASK_MODE;
    op.writeMask = mask;
    op.indices[0] = reg;
    op.indexDim   = 1;
    return op;
}

static D3D11SW_ParsedShader MakeMinimalVS()
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Vertex;
    s.numTemps = 1;

    D3D11SW_ShaderSignatureElement svPos{};
    std::strncpy(svPos.name, "SV_Position", 63);
    svPos.reg    = 0;
    svPos.svType = D3D_NAME_POSITION;
    svPos.mask   = 0xF;
    s.outputs.push_back(svPos);

    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_MOV,
        { MakeOutput(0), MakeInput(0) }));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));
    return s;
}

static SM4Operand MakeImm(Float x, Float y, Float z, Float w)
{
    SM4Operand op{};
    op.type   = D3D10_SB_OPERAND_TYPE_IMMEDIATE32;
    op.imm[0] = x;
    op.imm[1] = y;
    op.imm[2] = z;
    op.imm[3] = w;
    return op;
}

static SM4Operand MakeUAV(Uint32 slot)
{
    SM4Operand op{};
    op.type      = D3D11_SB_OPERAND_TYPE_UNORDERED_ACCESS_VIEW;
    op.compMode  = D3D10_SB_OPERAND_4_COMPONENT_MASK_MODE;
    op.writeMask = 0xF;
    op.indices[0] = slot;
    op.indexDim   = 1;
    return op;
}

static SM4Operand MakeResource(Uint32 slot)
{
    SM4Operand op{};
    op.type      = D3D10_SB_OPERAND_TYPE_RESOURCE;
    op.compMode  = D3D10_SB_OPERAND_4_COMPONENT_SWIZZLE_MODE;
    op.swizzle[0] = 0; op.swizzle[1] = 1; op.swizzle[2] = 2; op.swizzle[3] = 3;
    op.indices[0] = slot;
    op.indexDim   = 1;
    return op;
}

static SM4Operand MakeSampler(Uint32 slot)
{
    SM4Operand op{};
    op.type      = D3D10_SB_OPERAND_TYPE_SAMPLER;
    op.indices[0] = slot;
    op.indexDim   = 1;
    return op;
}

static D3D11SW_ParsedShader MakeCSShader(std::vector<SM4Instruction> instrs,
                                          Uint32 numTemps = 2)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Compute;
    s.numTemps = numTemps;
    s.instrs   = std::move(instrs);
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));
    return s;
}

struct ShaderCodeGenTests : ::testing::Test {};

TEST_F(ShaderCodeGenTests, VSSignature)
{
    std::string src = EmitShader(MakeMinimalVS(), "shaders/shader_runtime.h");
    EXPECT_NE(src.find("SW_VSInput"), std::string::npos);
    EXPECT_NE(src.find("SW_VSOutput"), std::string::npos);
    EXPECT_NE(src.find("ShaderMain"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, PSSignature)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Pixel;
    s.numTemps = 1;
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("SW_PSInput"), std::string::npos);
    EXPECT_NE(src.find("SW_PSOutput"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, CSSignature)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Compute;
    s.numTemps = 1;
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("SW_CSInput"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, IncludeRuntimeHeader)
{
    std::string src = EmitShader(MakeMinimalVS(), "shaders/shader_runtime.h");
    EXPECT_NE(src.find("#include \"shaders/shader_runtime.h\""), std::string::npos);
}

TEST_F(ShaderCodeGenTests, TempArraySize)
{
    D3D11SW_ParsedShader s = MakeMinimalVS();
    s.numTemps = 4;

    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("r[4]"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, MovEmitted)
{
    std::string src = EmitShader(MakeMinimalVS(), "shaders/shader_runtime.h");
    EXPECT_NE(src.find("in_ptr->v[0]"), std::string::npos);
    EXPECT_NE(src.find("out_v[0]"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, SVPositionMappedToPos)
{
    std::string src = EmitShader(MakeMinimalVS(), "shaders/shader_runtime.h");
    EXPECT_NE(src.find("out_ptr->pos"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, IfNonZero)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Vertex;
    s.numTemps = 1;

    SM4Instruction ifInstr = MakeInstr(D3D10_SB_OPCODE_IF, { MakeTemp(0) });
    ifInstr.testNonZero = true;
    s.instrs.push_back(ifInstr);
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ENDIF));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("!= 0u"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, IfZero)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Vertex;
    s.numTemps = 1;

    SM4Instruction ifInstr = MakeInstr(D3D10_SB_OPCODE_IF, { MakeTemp(0) });
    ifInstr.testNonZero = false;
    s.instrs.push_back(ifInstr);
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ENDIF));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("== 0u"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, Dp4Emitted)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Vertex;
    s.numTemps = 2;

    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_DP4,
        { MakeTemp(0, 0x1), MakeTemp(1), MakeTemp(1) }));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_dot4"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, SaturateWraps)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Vertex;
    s.numTemps = 2;

    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_MUL,
        { MakeTemp(0), MakeTemp(1), MakeTemp(1) }, true));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_saturate"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, BreakEmitted)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Vertex;
    s.numTemps = 1;
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_LOOP));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_BREAK));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ENDLOOP));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("break;"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, BreakcNonZero)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Vertex;
    s.numTemps = 1;

    SM4Instruction bc = MakeInstr(D3D10_SB_OPCODE_BREAKC, { MakeTemp(0) });
    bc.testNonZero = true;
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_LOOP));
    s.instrs.push_back(bc);
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ENDLOOP));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("!= 0u"), std::string::npos);
    EXPECT_NE(src.find("break;"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, BreakcZero)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Vertex;
    s.numTemps = 1;

    SM4Instruction bc = MakeInstr(D3D10_SB_OPCODE_BREAKC, { MakeTemp(0) });
    bc.testNonZero = false;
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_LOOP));
    s.instrs.push_back(bc);
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ENDLOOP));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("== 0u"), std::string::npos);
    EXPECT_NE(src.find("break;"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, AddEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_ADD, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find(" + "), std::string::npos);
}

TEST_F(ShaderCodeGenTests, MulEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_MUL, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find(" * "), std::string::npos);
}

TEST_F(ShaderCodeGenTests, MadEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_MAD, { MakeTemp(0), MakeTemp(1), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find(" * "), std::string::npos);
    EXPECT_NE(src.find(" + "), std::string::npos);
}

TEST_F(ShaderCodeGenTests, DivEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_DIV, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find(" / "), std::string::npos);
}

TEST_F(ShaderCodeGenTests, Dp2Emitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_DP2, { MakeTemp(0, 0x1), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_dot2"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, Dp3Emitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_DP3, { MakeTemp(0, 0x1), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_dot3"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, SqrtEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_SQRT, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_sqrt"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, RsqEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_RSQ, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_rsq"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, LogEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_LOG, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_log2"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, ExpEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_EXP, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_exp2"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, FrcEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_FRC, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_frc"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, RoundNeEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_ROUND_NE, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_round_ne"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, RoundZEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_ROUND_Z, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_round_z"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, IaddEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_IADD, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_iadd"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, IshlEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_ISHL, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_ishl"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, IshrEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_ISHR, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_ishr"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, UshrEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_USHR, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_ushr"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, InegEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_INEG, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_ineg"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, AndEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_AND, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_and"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, OrEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_OR, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_or"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, XorEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_XOR, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_xor"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, NotEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_NOT, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_not"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, FtoiEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_FTOI, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_ftoi"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, ItofEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_ITOF, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_itof"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, UtofEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_UTOF, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_utof"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, FtouEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_FTOU, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_ftou"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, MinEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_MIN, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_min("), std::string::npos);
}

TEST_F(ShaderCodeGenTests, MaxEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_MAX, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_max("), std::string::npos);
}

TEST_F(ShaderCodeGenTests, IminEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_IMIN, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_imin"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, ImaxEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_IMAX, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_imax"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, UminEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_UMIN, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_umin"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, UmaxEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_UMAX, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_umax"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, MovcEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_MOVC, { MakeTemp(0), MakeTemp(1), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("!= 0u"), std::string::npos);
    EXPECT_NE(src.find("?"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, ElseEmitted)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Compute;
    s.numTemps = 1;

    SM4Instruction ifInstr = MakeInstr(D3D10_SB_OPCODE_IF, { MakeTemp(0) });
    ifInstr.testNonZero = true;
    s.instrs.push_back(ifInstr);
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ELSE));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ENDIF));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("else"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, StoreUavTypedEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_STORE_UAV_TYPED, { MakeUAV(0), MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_uav_store_typed"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, LdUavTypedEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_LD_UAV_TYPED, { MakeTemp(0), MakeTemp(1), MakeUAV(0) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_uav_load_typed"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, StoreRawEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_STORE_RAW, { MakeUAV(0), MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_uav_store_raw"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, LdRawEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_LD_RAW, { MakeTemp(0), MakeTemp(1), MakeUAV(0) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_uav_load_raw"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, StoreStructuredEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_STORE_STRUCTURED,
            { MakeUAV(0), MakeTemp(0), MakeTemp(1), MakeTemp(2) })
    }, 3);
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_uav_store_structured"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, LdStructuredEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_LD_STRUCTURED,
            { MakeTemp(0), MakeTemp(1), MakeTemp(2), MakeUAV(0) })
    }, 3);
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_uav_load_structured"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, SampleEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_SAMPLE,
            { MakeTemp(0), MakeTemp(1), MakeResource(0), MakeSampler(0) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_sample_2d"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, LdTexEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_LD, { MakeTemp(0), MakeTemp(1), MakeResource(0) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_fetch_texel"), std::string::npos);
}

static SM4Operand MakeNull()
{
    SM4Operand op{};
    op.type = D3D10_SB_OPERAND_TYPE_NULL;
    return op;
}

TEST_F(ShaderCodeGenTests, EqEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_EQ, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_feq"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, NeEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_NE, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_fne"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, GeEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_GE, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_fge"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, LtEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_LT, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_flt"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, ImulLoEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_IMUL, { MakeNull(), MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_imul_lo"), std::string::npos);
    EXPECT_EQ(src.find("sw_imul_hi"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, ImulHiEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_IMUL, { MakeTemp(0), MakeNull(), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_imul_hi"), std::string::npos);
    EXPECT_EQ(src.find("sw_imul_lo"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, ImulBothEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_IMUL, { MakeTemp(0), MakeTemp(1), MakeTemp(1), MakeTemp(1) })
    }, 3);
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_imul_hi"), std::string::npos);
    EXPECT_NE(src.find("sw_imul_lo"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, SincosSinEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_SINCOS, { MakeTemp(0), MakeNull(), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_sin"), std::string::npos);
    EXPECT_EQ(src.find("sw_cos"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, SincosCosEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_SINCOS, { MakeNull(), MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_cos"), std::string::npos);
    EXPECT_EQ(src.find("sw_sin"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, SincosBothEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_SINCOS, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_sin"), std::string::npos);
    EXPECT_NE(src.find("sw_cos"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, UdivQuotientEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_UDIV, { MakeTemp(0), MakeNull(), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_udiv"), std::string::npos);
    EXPECT_EQ(src.find("sw_umod"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, UdivRemainderEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_UDIV, { MakeNull(), MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_umod"), std::string::npos);
    EXPECT_EQ(src.find("sw_udiv"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, RetEmitted)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Compute;
    s.numTemps = 1;
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("return;"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, IeqEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_IEQ, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_ieq"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, UgeEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_UGE, { MakeTemp(0), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_uge"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, LoopEmitted)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Compute;
    s.numTemps = 1;
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_LOOP));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_ENDLOOP));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("while (true)"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, RoundNiEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_ROUND_NI, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_round_ni"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, RoundPiEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_ROUND_PI, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_round_pi"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, ImadEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D10_SB_OPCODE_IMAD, { MakeTemp(0), MakeTemp(1), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_imad"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, CountbitsEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_COUNTBITS, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_countbits"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, FirstbitHiEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_FIRSTBIT_HI, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_firstbit_hi"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, FirstbitLoEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_FIRSTBIT_LO, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_firstbit_lo"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, FirstbitShiEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_FIRSTBIT_SHI, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_firstbit_shi"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, BfrevEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_BFREV, { MakeTemp(0), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_bfrev"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, UbfeEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_UBFE, { MakeTemp(0), MakeTemp(1), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_ubfe"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, IbfeEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_IBFE, { MakeTemp(0), MakeTemp(1), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_ibfe"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, BfiEmitted)
{
    auto s = MakeCSShader({
        MakeInstr(D3D11_SB_OPCODE_BFI, { MakeTemp(0), MakeTemp(1), MakeTemp(1), MakeTemp(1), MakeTemp(1) })
    });
    std::string src = EmitShader(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("sw_bfi"), std::string::npos);
}
