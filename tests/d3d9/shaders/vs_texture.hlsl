// Vertex shader for textured draws: passes POSITION and TEXCOORD through.

struct VSInput
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct VSOutput
{
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

VSOutput main(VSInput inp)
{
    VSOutput o;
    o.pos = float4(inp.pos, 1.0);
    o.uv  = inp.uv;
    return o;
}
