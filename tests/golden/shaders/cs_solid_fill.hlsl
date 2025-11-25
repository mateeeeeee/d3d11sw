RWBuffer<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    output[idx] = float4(0.25, 0.5, 0.75, 1.0);
}
