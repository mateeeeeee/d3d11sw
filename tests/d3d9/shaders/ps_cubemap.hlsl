samplerCUBE s0 : register(s0);

struct PSInput
{
    float4 dir : TEXCOORD0;
};

float4 main(PSInput inp) : COLOR
{
    return texCUBE(s0, inp.dir.xyz);
}
