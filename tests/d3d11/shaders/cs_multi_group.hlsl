RWBuffer<float4> output : register(u0);

[numthreads(4, 4, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    output[idx] = float4(DTid.x / 7.0, DTid.y / 7.0, 0.5, 1.0);
}
