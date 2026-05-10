struct DS_INPUT
{
    float4 pos : SV_Position;
};

struct HS_CONSTANT_OUTPUT
{
    float edges[3] : SV_TessFactor;
    float inside   : SV_InsideTessFactor;
};

struct DS_OUTPUT
{
    float4 pos : SV_Position;
};

[domain("tri")]
DS_OUTPUT main(HS_CONSTANT_OUTPUT patchConst,
               float3 uvw : SV_DomainLocation,
               const OutputPatch<DS_INPUT, 3> patch)
{
    DS_OUTPUT o;
    float3 finalPos = patch[0].pos.xyz * uvw.x +
                      patch[1].pos.xyz * uvw.y +
                      patch[2].pos.xyz * uvw.z;
    o.pos = float4(finalPos, 1.0);
    return o;
}
