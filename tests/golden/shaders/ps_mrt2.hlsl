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
    o.color0 = float4(1.0, 0.0, 0.0, 1.0);
    o.color1 = float4(0.0, 0.0, 1.0, 1.0);
    return o;
}
