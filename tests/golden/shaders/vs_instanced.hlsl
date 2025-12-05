struct VSInput
{
    float3 pos : POSITION;
    uint instanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 pos : SV_Position;
};

VSOutput main(VSInput inp)
{
    VSOutput o;
    float xOffset = (float)inp.instanceID - 0.5;
    o.pos = float4(inp.pos.x * 0.5 + xOffset, inp.pos.y, inp.pos.z, 1.0);
    return o;
}
