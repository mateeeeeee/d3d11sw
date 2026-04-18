struct HS_INPUT
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
};

struct HS_OUTPUT
{
    float3 pos   : POSITION;
    float4 color : COLOR;
};

struct HS_CONSTANT_OUTPUT
{
    float edges[3] : SV_TessFactor;
    float inside   : SV_InsideTessFactor;
};

HS_CONSTANT_OUTPUT ConstantHS(InputPatch<HS_INPUT, 3> patch)
{
    HS_CONSTANT_OUTPUT o;
    o.edges[0] = 4.0;
    o.edges[1] = 4.0;
    o.edges[2] = 4.0;
    o.inside   = 4.0;
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
    o.pos   = patch[id].pos.xyz;
    o.color = patch[id].color;
    return o;
}
