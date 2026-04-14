#pragma once
#include "common/common.h"

namespace d3d11sw {

//https://timjones.io/blog/archive/2015/09/02/parsing-direct3d-shader-bytecode

// DXBC container layout  (all values little-endian)
// File layout:
//   DXBCContainerHeader          (32 bytes)
//   UINT32 chunkOffsets[count]   (4 bytes each, offsets from file start)
//    chunk data 
// Each chunk:
//   DXBCChunkHeader  (8 bytes)
//   chunk payload (chunkHeader.size bytes) 

struct DXBCContainerHeader
{
    UINT32 magic;         // 'D','X','B','C' = 0x43425844
    UINT8  checksum[16];  
    UINT32 version;       
    UINT32 totalSize;     
    UINT32 chunkCount;
};

struct DXBCChunkHeader
{
    UINT32 fourCC;
    UINT32 size;   
};

static constexpr UINT32 MakeFourCC(char a, char b, char c, char d)
{
    return (UINT32)a | ((UINT32)b << 8) | ((UINT32)c << 16) | ((UINT32)d << 24);
}

static constexpr UINT32 DXBC_MAGIC = MakeFourCC('D','X','B','C');

static constexpr UINT32 FOURCC_SHDR = MakeFourCC('S','H','D','R'); // SM4 shader body
static constexpr UINT32 FOURCC_SHEX = MakeFourCC('S','H','E','X'); // SM5 shader body
static constexpr UINT32 FOURCC_ISGN = MakeFourCC('I','S','G','N'); // input signature
static constexpr UINT32 FOURCC_OSGN = MakeFourCC('O','S','G','N'); // output signature (SM4)
static constexpr UINT32 FOURCC_OSG5 = MakeFourCC('O','S','G','5'); // output signature (SM5)
static constexpr UINT32 FOURCC_RDEF = MakeFourCC('R','D','E','F'); // resource/cbuf defs

// ISGN / OSGN / OSG5  signature chunk
// Payload layout:
//   UINT32 elementCount
//   UINT32 unknown  (always 8)
//   DXBCSigElement[elementCount]
//   name strings (null-terminated, packed) 
// nameOffset is relative to the start of the chunk payload (not the file).
struct DXBCSigElement
{
    UINT32 nameOffset;      // offset of semantic name string from chunk payload start
    UINT32 semanticIndex;
    UINT32 svType;          // D3D_NAME system-value type (0 = user semantic)
    UINT32 componentType;   // D3D_REGISTER_COMPONENT_TYPE
    UINT32 registerIndex;   // shader register (0xFF = no register)
    UINT8  mask;            // component mask (xyzw = bits 0-3)
    UINT8  rwMask;          // read/write mask
    UINT8  pad[2];
};
static_assert(sizeof(DXBCSigElement) == 24, "DXBCSigElement must be 24 bytes");

struct DXBCSigElement5
{
    UINT32 stream;
    UINT32 nameOffset;
    UINT32 semanticIndex;
    UINT32 svType;
    UINT32 componentType;
    UINT32 registerIndex;
    UINT8  mask;
    UINT8  rwMask;
    UINT8  pad[2];
};
static_assert(sizeof(DXBCSigElement5) == 28, "DXBCSigElement5 must be 28 bytes");

// RDEF  chunk  (resource / constant-buffer definitions)
// Payload layout:
//   DXBCRdefHeader
//   DXBCRdefCbuf[cbufCount]      at cbufOffset from payload start
//   DXBCRdefBinding[bindCount]   at bindingOffset from payload start
struct DXBCRdefHeader
{
    UINT32 cbufCount;
    UINT32 cbufOffset;      //from payload start
    UINT32 bindingCount;
    UINT32 bindingOffset;   //from payload start
    UINT8  minorVersion;
    UINT8  majorVersion;
    UINT16 shaderType;      
    UINT32 compilerFlags;
    UINT32 creatorOffset;   
};

struct DXBCRdefCbuf
{
    UINT32 nameOffset;
    UINT32 varCount;
    UINT32 varOffset;
    UINT32 sizeBytes;
    UINT32 flags;
    UINT32 bufType;
};

struct DXBCRdefBinding
{
    UINT32 nameOffset;
    UINT32 inputType;   //D3D_SHADER_INPUT_TYPE
    UINT32 returnType;
    UINT32 dimension;   //D3D_SRV_DIMENSION
    UINT32 numSamples;
    UINT32 bindPoint;
    UINT32 bindCount;
    UINT32 uFlags;
};

static constexpr UINT32 RDEF_SIT_CBUFFER = 0;
static constexpr UINT32 RDEF_SIT_TEXTURE = 2;
static constexpr UINT32 RDEF_SIT_SAMPLER = 3;

//RDEF shaderType field values (upper byte of the version word)
static constexpr UINT16 RDEF_SHTYPE_PS = 0xFFFF;
static constexpr UINT16 RDEF_SHTYPE_VS = 0xFFFE;
static constexpr UINT16 RDEF_SHTYPE_GS = 0xFFFD;
static constexpr UINT16 RDEF_SHTYPE_HS = 0xFFF8;  
static constexpr UINT16 RDEF_SHTYPE_DS = 0xFFF9;  
static constexpr UINT16 RDEF_SHTYPE_CS = 0xFFFA;  

} 
