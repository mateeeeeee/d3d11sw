cbuffer CB : register(b0)
{
    float4 color;
    float  depthVal;
};

struct PSOutput
{
    float4 color : SV_Target;
    float  depth : SV_Depth;
};

PSOutput main(float4 pos : SV_Position)
{
    PSOutput o;
    o.color = color;
    o.depth = depthVal;
    return o;
}
