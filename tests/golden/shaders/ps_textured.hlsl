Texture2D    tex : register(t0);
SamplerState smp : register(s0);

struct PSInput
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

float4 main(PSInput inp) : SV_Target
{
    return tex.Sample(smp, inp.uv);
}
