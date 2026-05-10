struct VSInput
{
    float3 pos    : POSITION;
    float2 offset : TEXCOORD0;
    float4 color  : COLOR;
};

struct VSOutput
{
    float4 pos   : POSITION;
    float4 color : COLOR;
};

VSOutput main(VSInput inp)
{
    VSOutput o;
    o.pos   = float4(inp.pos.x + inp.offset.x, inp.pos.y + inp.offset.y, inp.pos.z, 1.0);
    o.color = inp.color;
    return o;
}
