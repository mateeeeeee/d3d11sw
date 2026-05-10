// Pixel shader that outputs the interpolated input color.
// Pairs with vs_color.hlsl — together they verify VS→PS varying interpolation
// (Gouraud shading via SM3 v# input registers).

struct PSInput
{
    float4 color : COLOR;
};

float4 main(PSInput inp) : COLOR
{
    return inp.color;
}
