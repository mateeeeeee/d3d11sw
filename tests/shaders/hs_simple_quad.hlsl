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

HS_CONSTANT_OUTPUT ConstantHS(InputPatch<HS_INPUT, 4> patch)
{
    HS_CONSTANT_OUTPUT o;
    o.edges[0] = 3.0;
    o.edges[1] = 3.0;
    o.edges[2] = 3.0;
    o.edges[3] = 3.0;
    o.inside[0] = 3.0;
    o.inside[1] = 3.0;
    return o;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0)]
HS_OUTPUT main(InputPatch<HS_INPUT, 4> patch, uint id : SV_OutputControlPointID)
{
    HS_OUTPUT o;
    o.pos = patch[id].pos.xyz;
    return o;
}
