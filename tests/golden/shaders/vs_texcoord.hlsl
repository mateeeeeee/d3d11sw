struct VSInput
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

VSOutput main(VSInput inp)
{
    VSOutput o;
    o.pos = float4(inp.pos, 1.0);
    o.uv  = inp.uv;
    return o;
}
