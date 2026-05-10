Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target
{
    return tex.SampleLevel(smp, uv, 0);
}
