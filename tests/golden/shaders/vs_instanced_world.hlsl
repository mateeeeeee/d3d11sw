cbuffer CB : register(b0)
{
    float4x4 viewProj;
    float4x4 world[27];
};

struct VSInput
{
    float3 pos    : POSITION;
    float3 normal : NORMAL;
    float2 uv     : TEXCOORD;
    uint   instID : SV_InstanceID;
};

struct VSOutput
{
    float4 pos    : SV_Position;
    float3 normal : NORMAL;
    float2 uv     : TEXCOORD;
};

VSOutput main(VSInput inp)
{
    VSOutput o;
    float4 worldPos = mul(float4(inp.pos, 1.0), world[inp.instID]);
    o.pos    = mul(worldPos, viewProj);
    o.normal = mul(float4(inp.normal, 0.0), world[inp.instID]).xyz;
    o.uv     = inp.uv;
    return o;
}
