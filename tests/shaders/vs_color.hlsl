struct VSInput
{
    float3 pos   : POSITION;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
};

VSOutput main(VSInput inp)
{
    VSOutput o;
    o.pos   = float4(inp.pos, 1.0);
    o.color = inp.color;
    return o;
}
