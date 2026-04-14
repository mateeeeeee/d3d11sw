Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct PSIn
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

float4 main(PSIn i) : SV_Target
{
    float scale = (i.pos.x < 4.0) ? 1.0 : 4.0;
    return tex.Sample(smp, i.uv * scale);
}
