cbuffer ColorBuf : register(b0)
{
    float4 mainColor;
    float4 src1Color;
};

struct PSInput
{
    float4 pos : SV_Position;
};

struct PSOutput
{
    float4 color0 : SV_Target0;
    float4 color1 : SV_Target1;
};

PSOutput main(PSInput inp)
{
    PSOutput o;
    o.color0 = mainColor;
    o.color1 = src1Color;
    return o;
}
