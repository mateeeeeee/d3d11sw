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

struct ShaderCodeGenTests : ::testing::Test {};

TEST_F(ShaderCodeGenTests, VSSignature)
{
    ShaderCodeGen cg;
    std::string src = cg.Emit(MakeMinimalVS(), "shaders/shader_runtime.h");
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

    ShaderCodeGen cg;
    std::string src = cg.Emit(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("SW_PSInput"), std::string::npos);
    EXPECT_NE(src.find("SW_PSOutput"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, CSSignature)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Compute;
    s.numTemps = 1;
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    ShaderCodeGen cg;
    std::string src = cg.Emit(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("SW_CSInput"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, IncludeRuntimeHeader)
{
    ShaderCodeGen cg;
    std::string src = cg.Emit(MakeMinimalVS(), "shaders/shader_runtime.h");
    EXPECT_NE(src.find("#include \"shaders/shader_runtime.h\""), std::string::npos);
}

TEST_F(ShaderCodeGenTests, TempArraySize)
{
    D3D11SW_ParsedShader s = MakeMinimalVS();
    s.numTemps = 4;

    ShaderCodeGen cg;
    std::string src = cg.Emit(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("r[4]"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, MovEmitted)
{
    ShaderCodeGen cg;
    std::string src = cg.Emit(MakeMinimalVS(), "shaders/shader_runtime.h");
    EXPECT_NE(src.find("in_ptr->v[0]"), std::string::npos);
    EXPECT_NE(src.find("out_v[0]"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, SVPositionMappedToPos)
{
    ShaderCodeGen cg;
    std::string src = cg.Emit(MakeMinimalVS(), "shaders/shader_runtime.h");
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

    ShaderCodeGen cg;
    std::string src = cg.Emit(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("!= 0.f"), std::string::npos);
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

    ShaderCodeGen cg;
    std::string src = cg.Emit(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("== 0.f"), std::string::npos);
}

TEST_F(ShaderCodeGenTests, Dp4Emitted)
{
    D3D11SW_ParsedShader s{};
    s.type     = D3D11SW_ShaderType::Vertex;
    s.numTemps = 2;

    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_DP4,
        { MakeTemp(0, 0x1), MakeTemp(1), MakeTemp(1) }));
    s.instrs.push_back(MakeInstr(D3D10_SB_OPCODE_RET));

    ShaderCodeGen cg;
    std::string src = cg.Emit(s, "shaders/shader_runtime.h");
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

    ShaderCodeGen cg;
    std::string src = cg.Emit(s, "shaders/shader_runtime.h");
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

    ShaderCodeGen cg;
    std::string src = cg.Emit(s, "shaders/shader_runtime.h");
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

    ShaderCodeGen cg;
    std::string src = cg.Emit(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("!= 0.f"), std::string::npos);
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

    ShaderCodeGen cg;
    std::string src = cg.Emit(s, "shaders/shader_runtime.h");
    EXPECT_NE(src.find("== 0.f"), std::string::npos);
    EXPECT_NE(src.find("break;"), std::string::npos);
}
