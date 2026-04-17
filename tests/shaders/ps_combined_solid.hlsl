cbuffer cbPerFrame : register(b2)
{
    matrix World;
    float4 _pad[12];
    float4 outputColor;
};

struct PS_IN
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

float4 main(PS_IN input) : SV_Target
{
    return outputColor;
}
