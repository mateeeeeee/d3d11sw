Texture2D    tex : register(t0);
SamplerState smp : register(s0);

struct PSInput
{
    float4 pos    : SV_Position;
    float3 normal : NORMAL;
    float2 uv     : TEXCOORD;
};

float4 main(PSInput inp) : SV_Target
{
    float3 tint = inp.normal * 0.5 + 0.5;
    float4 texColor = tex.Sample(smp, inp.uv);
    return float4(texColor.rgb * tint, 1.0);
}
