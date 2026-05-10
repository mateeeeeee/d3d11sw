#pragma once
#include <cstring>
#include "d3d9/common/d3d9_headers.h"
#include "core/shaders/parsed_shader.h"

namespace d3dsw {

struct FFVSKey
{
    DWORD fvf              = 0;
    Uint  numTexCoords     = 0;
    Uint8 lightMask        = 0;      //bitmask of enabled lights (bits 0-7)
    Uint8 lightTypes[8]    = {};     //D3DLIGHTTYPE per slot (0 if not enabled)
    Uint8 lightingEnabled  = 0;      //D3DRS_LIGHTING && hasNormal
    Uint8 specularEnabled  = 0;      //D3DRS_SPECULARENABLE
    Uint8 colorVertex      = 0;
    Uint8 diffuseSrc       = 0;
    Uint8 specularSrc      = 0;
    Uint8 ambientSrc       = 0;
    Uint8 emissiveSrc      = 0;
    Uint8 fogVertexMode    = 0;   //D3DFOG_NONE=0, D3DFOG_LINEAR=1, D3DFOG_EXP=2, D3DFOG_EXP2=3
    Uint8 texTransformFlags[8] = {}; //D3DTTFF_* per texcoord stage; 0 = disable

    Bool operator==(const FFVSKey& o) const { return std::memcmp(this, &o, sizeof(*this)) == 0; }
    Bool operator< (const FFVSKey& o) const { return std::memcmp(this, &o, sizeof(*this)) <  0; }
};

struct FFPSKey
{
    struct StageKey
    {
        DWORD  colorOp    = D3DTOP_DISABLE;
        DWORD  colorArg1  = D3DTA_TEXTURE;
        DWORD  colorArg2  = D3DTA_CURRENT;
        DWORD  alphaOp    = D3DTOP_DISABLE;
        DWORD  alphaArg1  = D3DTA_TEXTURE;
        DWORD  alphaArg2  = D3DTA_CURRENT;
        Uint8  hasTexture = 0;
        Uint8  _pad[3]    = {};
    } stages[8];

    Uint8 hasDiffuse   = 0;
    Uint8 numTexCoords = 0;
    Uint8 _pad[2]      = {};

    Bool operator==(const FFPSKey& o) const { return std::memcmp(this, &o, sizeof(*this)) == 0; }
    Bool operator< (const FFPSKey& o) const { return std::memcmp(this, &o, sizeof(*this)) <  0; }
};

void SynthesizeFFVS(const FFVSKey& key, D3DSW_ParsedShader& out);
void SynthesizeFFPS(const FFPSKey& key, D3DSW_ParsedShader& out);

}
