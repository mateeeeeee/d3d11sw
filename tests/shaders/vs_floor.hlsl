cbuffer CB : register(b0)
{
    float4x4 viewProj;
};

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

VSOut main(float3 pos : POSITION, float2 uv : TEXCOORD0)
{
    VSOut o;
    o.pos = mul(float4(pos, 1), viewProj);
    o.uv  = uv;
    return o;
}
