cbuffer TessConstants : register(b0)
{
    float4   cameraPos;
    float4x4 viewProj;
};

struct DS_INPUT
{
    float3 pos : POSITION;
};

struct HS_CONSTANT_OUTPUT
{
    float edges[4]  : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

struct DS_OUTPUT
{
    float4 pos : SV_Position;
};

[domain("quad")]
DS_OUTPUT main(HS_CONSTANT_OUTPUT patchConst,
               float2 uv : SV_DomainLocation,
               const OutputPatch<DS_INPUT, 4> patch)
{
    float3 top    = lerp(patch[0].pos, patch[1].pos, uv.x);
    float3 bottom = lerp(patch[3].pos, patch[2].pos, uv.x);
    float3 worldPos = lerp(top, bottom, uv.y);

    DS_OUTPUT o;
    o.pos = mul(float4(worldPos, 1.0), viewProj);
    return o;
}
