struct VSInput
{
    float3 pos : POSITION;
    float4 tc  : TEXCOORD0;
};

struct VSOutput
{
    float4 pos : POSITION;
    float4 tc  : TEXCOORD0;
};

VSOutput main(VSInput inp)
{
    VSOutput o;
    o.pos = float4(inp.pos, 1.0);
    o.tc  = inp.tc;
    return o;
}
