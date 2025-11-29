cbuffer ColorBuf : register(b0)
{
    float4 solidColor;
};

struct PSInput
{
    float4 pos : SV_Position;
};

float4 main(PSInput inp) : SV_Target
{
    return solidColor;
}
