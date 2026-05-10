sampler3D s0 : register(s0);

struct PSInput
{
    float4 uvw : TEXCOORD0;
};

float4 main(PSInput inp) : COLOR
{
    return tex3D(s0, inp.uvw.xyz);
}
