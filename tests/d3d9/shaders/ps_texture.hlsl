// Pixel shader that samples texture stage 0 using tex-coord input v0.
// Pairs with vs_color.hlsl (outputs tex-coord as COLOR semantic) or
// any VS that outputs TEXCOORD0.

sampler2D s0 : register(s0);

struct PSInput
{
    float2 uv : TEXCOORD0;
};

float4 main(PSInput inp) : COLOR
{
    return tex2D(s0, inp.uv);
}
