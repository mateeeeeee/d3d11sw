#pragma once
#include "common/common.h"
#include <vector>

namespace d3d11sw {

// https://github.com/tpn/winsdk-10/blob/master/Include/10.0.16299.0/um/d3d10TokenizedProgramFormat.hpp
// https://github.com/jingoro2112/dna/blob/master/dnausb/win32/ddk/api/d3d11TokenizedProgramFormat.hpp
enum class SM4OpCode : Uint16
{
    // SM4 / D3D10 opcodes (0-87)
    Add                         = 0,
    And                         = 1,
    Break                       = 2,
    Breakc                      = 3,
    Call                        = 4,
    Callc                       = 5,
    Case                        = 6,
    Continue                    = 7,
    Continuec                   = 8,
    Cut                         = 9,
    Default                     = 10,
    DerivRtx                    = 11,
    DerivRty                    = 12,
    Discard                     = 13,
    Div                         = 14,
    Dp2                         = 15,
    Dp3                         = 16,
    Dp4                         = 17,
    Else                        = 18,
    Emit                        = 19,
    EmitThenCut                 = 20,
    Endif                       = 21,
    Endloop                     = 22,
    Endswitch                   = 23,
    Eq                          = 24,
    Exp                         = 25,
    Frc                         = 26,
    Ftoi                        = 27,
    Ftou                        = 28,
    Ge                          = 29,
    Iadd                        = 30,
    If                          = 31,
    Ieq                         = 32,
    Ige                         = 33,
    Ilt                         = 34,
    Imad                        = 35,
    Imax                        = 36,
    Imin                        = 37,
    Imul                        = 38,
    Ine                         = 39,
    Ineg                        = 40,
    Ishl                        = 41,
    Ishr                        = 42,
    Itof                        = 43,
    Label                       = 44,
    Ld                          = 45,
    LdMs                        = 46,
    Log                         = 47,
    Loop                        = 48,
    Lt                          = 49,
    Mad                         = 50,
    Min                         = 51,
    Max                         = 52,
    CustomData                  = 53,
    Mov                         = 54,
    Movc                        = 55,
    Mul                         = 56,
    Ne                          = 57,
    Nop                         = 58,
    Not                         = 59,
    Or                          = 60,
    ResInfo                     = 61,
    Ret                         = 62,
    Retc                        = 63,
    RoundNe                     = 64,
    RoundNi                     = 65,
    RoundPi                     = 66,
    RoundZ                      = 67,
    Rsq                         = 68,
    Sample                      = 69,
    SampleC                     = 70,
    SampleCLz                   = 71,
    SampleL                     = 72,
    SampleD                     = 73,
    SampleB                     = 74,
    Sqrt                        = 75,
    Switch                      = 76,
    Sincos                      = 77,
    UDiv                        = 78,
    ULt                         = 79,
    UGe                         = 80,
    Umul                        = 81,
    Umad                        = 82,
    Umax                        = 83,
    Umin                        = 84,
    Ushr                        = 85,
    Utof                        = 86,
    Xor                         = 87,

    // SM4 declarations (88-106)
    DclResource                 = 88,
    DclConstantBuffer           = 89,
    DclSampler                  = 90,
    DclIndexRange               = 91,
    DclGsOutputPrimitiveTopo    = 92,
    DclGsInputPrimitive         = 93,
    DclMaxOutputVertexCount     = 94,
    DclInput                    = 95,
    DclInputSgv                 = 96,
    DclInputSiv                 = 97,
    DclInputPs                  = 98,
    DclInputPsSgv               = 99,
    DclInputPsSiv               = 100,
    DclOutput                   = 101,
    DclOutputSgv                = 102,
    DclOutputSiv                = 103,
    DclTemps                    = 104,
    DclIndexableTemp            = 105,
    DclGlobalFlags              = 106,

    Reserved0                   = 107,

    // D3D10.1 additions (108-112)
    Lod                         = 108,
    Gather4                     = 109,
    SamplePos                   = 110,
    SampleInfo                  = 111,
    Reserved1                   = 112,

    // SM5 / D3D11 opcodes (113+)
    HsDecls                     = 113,
    HsControlPointPhase         = 114,
    HsForkPhase                 = 115,
    HsJoinPhase                 = 116,
    EmitStream                  = 117,
    CutStream                   = 118,
    EmitThenCutStream           = 119,
    InterfaceCall               = 120,
    Bufinfo                     = 121,
    DerivRtxCoarse              = 122,
    DerivRtxFine                = 123,
    DerivRtyCoarse              = 124,
    DerivRtyFine                = 125,
    Gather4C                    = 126,
    Gather4Po                   = 127,
    Gather4PoC                  = 128,
    Rcp                         = 129,
    F32toF16                    = 130,
    F16toF32                    = 131,
    Uaddc                       = 132,
    Usubb                       = 133,
    Countbits                   = 134,
    FirstbitHi                  = 135,
    FirstbitLo                  = 136,
    FirstbitShi                 = 137,
    Ubfe                        = 138,
    Ibfe                        = 139,
    Bfi                         = 140,
    Bfrev                       = 141,
    Swapc                       = 142,
    DclStream                   = 143,
    DclFunctionBody             = 144,
    DclFunctionTable            = 145,
    DclInterface                = 146,
    DclInputControlPointCount   = 147,
    DclOutputControlPointCount  = 148,
    DclTessDomain               = 149,
    DclTessPartitioning         = 150,
    DclTessOutputPrimitive      = 151,
    DclHsMaxTessFactor          = 152,
    DclHsForkPhaseInstanceCount = 153,
    DclHsJoinPhaseInstanceCount = 154,
    DclThreadGroup              = 155, 
    DclUavTyped                 = 156,
    DclUavRaw                   = 157,
    DclUavStructured            = 158,
    DclTgsRaw                   = 159, 
    DclTgsStructured            = 160, 
    DclResourceRaw              = 161,
    DclResourceStructured       = 162,
    LdUavTyped                  = 163,
    StoreUavTyped               = 164,
    LdRaw                       = 165,
    StoreRaw                    = 166,
    LdStructured                = 167,
    StoreStructured             = 168,
    AtomicAnd                   = 169,
    AtomicOr                    = 170,
    AtomicXor                   = 171,
    AtomicCmpStore              = 172,
    AtomicIadd                  = 173,
    AtomicImax                  = 174,
    AtomicImin                  = 175,
    AtomicUmax                  = 176,
    AtomicUmin                  = 177,
    ImmAtomicAlloc              = 178,
    ImmAtomicConsume            = 179,
    ImmAtomicIadd               = 180,
    ImmAtomicAnd                = 181,
    ImmAtomicOr                 = 182,
    ImmAtomicXor                = 183,
    ImmAtomicExch               = 184,
    ImmAtomicCmpExch            = 185,
    ImmAtomicImax               = 186,
    ImmAtomicImin               = 187,
    ImmAtomicUmax               = 188,
    ImmAtomicUmin               = 189,
    Sync                        = 190,
    Dadd                        = 191,
    Dmax                        = 192,
    Dmin                        = 193,
    Dmul                        = 194,
    Deq                         = 195,
    Dge                         = 196,
    Dlt                         = 197,
    Dne                         = 198,
    Dmov                        = 199,
    Dmovc                       = 200,
    Dtof                        = 201,
    Ftod                        = 202,
    EvalSnapped                 = 203,
    EvalSampleIndex             = 204,
    EvalCentroid                = 205,
    DclGsInstanceCount          = 206,

    Unknown                     = 0xFFFF,
};

enum class SM4OperandType : UINT8
{
    Temp            = 0,
    Input           = 1,
    Output          = 2,
    IndexableTemp   = 3,
    Immediate32     = 4,
    Immediate64     = 5,
    Sampler         = 6,
    Resource        = 7,
    ConstantBuffer  = 8,
    ImmConstBuf     = 9,
    Label           = 10,
    InputPrimId     = 11,
    OutputDepth     = 12,
    Null            = 13,
    Unknown         = 0xFF,
};

// D3D10_SB_OPERAND_4_COMPONENT_SELECTION_MODE
enum class SM4CompMode : Uint8
{
    Mask    = 0,  //destination write mask
    Swizzle = 1,  //source swizzle
    Select1 = 2,  //source single-component select
};

struct SM4Operand
{
    SM4OperandType type         = SM4OperandType::Unknown;
    SM4CompMode    compMode     = SM4CompMode::Swizzle;
    Uint8          writeMask    = 0xF;       // bits 0-3 = xyzw; destination operands
    Uint8          swizzle[4]   = {0,1,2,3}; // source swizzle: which src component maps to xyzw
    Uint8          indexDim     = 0;         // number of index dimensions (0, 1, 2)
    Uint32         indices[2]   = {};        // e.g. cb0[4] -> {0, 4}
    Float          imm[4]       = {};        // Immediate32 values
    Bool           negate       = false;
    Bool           absolute     = false;
};

struct SM4Instruction
{
    SM4OpCode               op       = SM4OpCode::Unknown;
    Bool                    saturate = false;
    Uint32                  threadGroupSize[3] = {1, 1, 1}; 
    std::vector<SM4Operand> operands;
};

} 
