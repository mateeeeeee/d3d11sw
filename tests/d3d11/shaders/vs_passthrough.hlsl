struct VSInput
{
    float3 pos : POSITION;
};

struct VSOutput
{
    float4 pos : SV_Position;
};

VSOutput main(VSInput inp)
{
    VSOutput o;
    o.pos = float4(inp.pos, 1.0);
    return o;
}
