#include "d3d9/translate/fvf.h"

namespace d3dsw {

static void Push(std::vector<D3DVERTEXELEMENT9>& out, WORD stream, WORD& offset,
                 BYTE type, WORD size, BYTE usage, BYTE usageIndex)
{
    D3DVERTEXELEMENT9 e = {};
    e.Stream     = stream;
    e.Offset     = offset;
    e.Type       = type;
    e.Method     = D3DDECLMETHOD_DEFAULT;
    e.Usage      = usage;
    e.UsageIndex = usageIndex;
    out.push_back(e);
    offset = static_cast<WORD>(offset + size);
}

static void AppendTerminator(std::vector<D3DVERTEXELEMENT9>& out)
{
    D3DVERTEXELEMENT9 end = D3DDECL_END();
    out.push_back(end);
}

void FVFToDeclaration(DWORD fvf, std::vector<D3DVERTEXELEMENT9>& out)
{
    out.clear();
    WORD stream = 0;
    WORD offset = 0;

    const DWORD posType = fvf & D3DFVF_POSITION_MASK;
    switch (posType)
    {
        case D3DFVF_XYZ:
            Push(out, stream, offset, D3DDECLTYPE_FLOAT3, 12, D3DDECLUSAGE_POSITION, 0);
            break;
        case D3DFVF_XYZRHW:
            Push(out, stream, offset, D3DDECLTYPE_FLOAT4, 16, D3DDECLUSAGE_POSITIONT, 0);
            break;
        case D3DFVF_XYZW:
            Push(out, stream, offset, D3DDECLTYPE_FLOAT4, 16, D3DDECLUSAGE_POSITION, 0);
            break;
        case D3DFVF_XYZB1:
        case D3DFVF_XYZB2:
        case D3DFVF_XYZB3:
        case D3DFVF_XYZB4:
        case D3DFVF_XYZB5:
        {
            Push(out, stream, offset, D3DDECLTYPE_FLOAT3, 12, D3DDECLUSAGE_POSITION, 0);
            // XYZBn has n blend weights after XYZ. n in [1..5].
            const Int n = (static_cast<Int>(posType) - D3DFVF_XYZB1) / 2 + 1;
            if (n >= 1)
            {
                const BYTE weightTypes[] = { D3DDECLTYPE_FLOAT1, D3DDECLTYPE_FLOAT2,
                                             D3DDECLTYPE_FLOAT3, D3DDECLTYPE_FLOAT4,
                                             D3DDECLTYPE_FLOAT4 };
                const WORD weightSizes[] = { 4, 8, 12, 16, 16 };
                const int idx = (n > 5) ? 4 : (n - 1);
                Push(out, stream, offset, weightTypes[idx], weightSizes[idx],
                     D3DDECLUSAGE_BLENDWEIGHT, 0);
            }
            break;
        }
        default:
            break;  
    }

    if (fvf & D3DFVF_NORMAL)
    {
        Push(out, stream, offset, D3DDECLTYPE_FLOAT3, 12, D3DDECLUSAGE_NORMAL, 0);
    }
    if (fvf & D3DFVF_PSIZE)
    {
        Push(out, stream, offset, D3DDECLTYPE_FLOAT1, 4, D3DDECLUSAGE_PSIZE, 0);
    }
    if (fvf & D3DFVF_DIFFUSE)
    {
        Push(out, stream, offset, D3DDECLTYPE_D3DCOLOR, 4, D3DDECLUSAGE_COLOR, 0);
    }
    if (fvf & D3DFVF_SPECULAR)
    {
        Push(out, stream, offset, D3DDECLTYPE_D3DCOLOR, 4, D3DDECLUSAGE_COLOR, 1);
    }

    const DWORD ntc = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    for (DWORD i = 0; i < ntc && i < 8; ++i)
    {
        const DWORD sizeBits = (fvf >> (16 + i * 2)) & 0x03;
        BYTE declType = D3DDECLTYPE_FLOAT2;
        WORD size     = 8;
        switch (sizeBits)
        {
            case D3DFVF_TEXTUREFORMAT2: declType = D3DDECLTYPE_FLOAT2; size = 8;  break;
            case D3DFVF_TEXTUREFORMAT3: declType = D3DDECLTYPE_FLOAT3; size = 12; break;
            case D3DFVF_TEXTUREFORMAT4: declType = D3DDECLTYPE_FLOAT4; size = 16; break;
            case D3DFVF_TEXTUREFORMAT1: declType = D3DDECLTYPE_FLOAT1; size = 4;  break;
        }
        Push(out, stream, offset, declType, size, D3DDECLUSAGE_TEXCOORD, static_cast<BYTE>(i));
    }

    AppendTerminator(out);
}

}
