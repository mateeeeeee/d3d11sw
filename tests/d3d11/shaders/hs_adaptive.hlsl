cbuffer TessConstants : register(b0)
{
    float4   cameraPos;
    float4x4 viewProj;
};

struct HS_INPUT
{
    float4 pos : SV_Position;
};

struct HS_OUTPUT
{
    float3 pos : POSITION;
};

struct HS_CONSTANT_OUTPUT
{
    float edges[4]  : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

static const float MAX_TESS = 16.0;
static const float REF_DIST = 2.0;

float ComputeEdgeTess(float3 p0, float3 p1)
{
    float3 mid = (p0 + p1) * 0.5;
    float dist = distance(mid, cameraPos.xyz);
    return clamp(MAX_TESS * REF_DIST / dist, 1.0, MAX_TESS);
}

HS_CONSTANT_OUTPUT ConstantHS(InputPatch<HS_INPUT, 4> patch)
{
    float3 p0 = patch[0].pos.xyz;
    float3 p1 = patch[1].pos.xyz;
    float3 p2 = patch[2].pos.xyz;
    float3 p3 = patch[3].pos.xyz;

    HS_CONSTANT_OUTPUT o;
    o.edges[0] = ComputeEdgeTess(p0, p3);
    o.edges[1] = ComputeEdgeTess(p0, p1);
    o.edges[2] = ComputeEdgeTess(p1, p2);
    o.edges[3] = ComputeEdgeTess(p2, p3);
    o.inside[0] = (o.edges[1] + o.edges[3]) * 0.5;
    o.inside[1] = (o.edges[0] + o.edges[2]) * 0.5;
    return o;
}

[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(16.0)]
HS_OUTPUT main(InputPatch<HS_INPUT, 4> patch, uint id : SV_OutputControlPointID)
{
    HS_OUTPUT o;
    o.pos = patch[id].pos.xyz;
    return o;
}
