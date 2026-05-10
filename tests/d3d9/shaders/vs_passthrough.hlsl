// Pass-through vertex shader: float3 POSITION → float4 POSITION (w=1).
// Smallest valid SM3 VS — the SM4-equivalent of `vs_passthrough.hlsl` in
// tests/d3d11/shaders, but written for SM3 (no SV_ prefix).

struct VSInput
{
    float3 pos : POSITION;
};

struct VSOutput
{
    float4 pos : POSITION;
};

VSOutput main(VSInput inp)
{
    VSOutput o;
    o.pos = float4(inp.pos, 1.0);
    return o;
}
