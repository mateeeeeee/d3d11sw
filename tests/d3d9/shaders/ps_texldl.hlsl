// Exercises TEXLDL: samples texture s0 with explicit LOD from c0.x.
// With LOD=0 and a 4×4 checkerboard, the result matches regular sampling.
sampler2D s0 : register(s0);
float4    g_lod : register(c0);  // x = explicit LOD level

struct PSInput { float2 uv : TEXCOORD0; };

float4 main(PSInput inp) : COLOR
{
    return tex2Dlod(s0, float4(inp.uv, 0.0, g_lod.x));
}
