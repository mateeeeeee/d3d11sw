struct PSIn
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

float4 main(PSIn i) : SV_Target
{
    float dx = ddx(i.uv.x);
    float dy = ddy(i.uv.y);
    return float4(i.uv.x, i.uv.y, dx * 8.0 + dy * 8.0, 1);
}
