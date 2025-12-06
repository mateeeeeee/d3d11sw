cbuffer CB : register(b0) { float4x4 mvp; };

struct VSInput
{
    float3 pos    : POSITION;
    float3 normal : NORMAL;
    float2 uv     : TEXCOORD;
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
    o.pos    = mul(float4(inp.pos, 1.0), mvp);
    o.normal = inp.normal;
    o.uv     = inp.uv;
    return o;
}
