struct PSInput
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
};

float4 main(PSInput inp) : SV_Target
{
    return inp.color;
}
