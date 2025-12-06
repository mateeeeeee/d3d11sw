struct PSInput
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
};

float4 main(PSInput inp) : SV_Target
{
    clip(inp.color.g - 0.5);
    return inp.color;
}
