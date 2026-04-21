struct HS_INPUT
{
    float4 pos : SV_Position;
};

struct HS_OUTPUT
{
    float4 pos : SV_Position;
};

struct HS_CONSTANT_OUTPUT
{
    float edges[3] : SV_TessFactor;
    float inside   : SV_InsideTessFactor;
};

HS_CONSTANT_OUTPUT ConstantHS(InputPatch<HS_INPUT, 3> patch)
{
    HS_CONSTANT_OUTPUT o;
    o.edges[0] = 2.0;
    o.edges[1] = 2.0;
    o.edges[2] = 2.0;
    o.inside   = 2.0;
    return o;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0)]
HS_OUTPUT main(InputPatch<HS_INPUT, 3> patch, uint id : SV_OutputControlPointID)
{
    HS_OUTPUT o;
    o.pos = float4(patch[id].pos.xyz, 1.0);
    return o;
}
