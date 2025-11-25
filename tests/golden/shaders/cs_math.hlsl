RWBuffer<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    float u = DTid.x / 7.0;
    float v = DTid.y / 7.0;
    float r = sqrt(u);
    float g = frac(v * 3.0);
    float b = saturate(u * 2.0 - 0.5);
    output[idx] = float4(r, g, b, 1.0);
}
