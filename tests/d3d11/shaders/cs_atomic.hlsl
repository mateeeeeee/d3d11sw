RWBuffer<float4> output : register(u0);
groupshared uint shared_sum;

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex)
{
    if (GI == 0)
    {
        shared_sum = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    InterlockedAdd(shared_sum, GI + 1);

    GroupMemoryBarrierWithGroupSync();

    uint idx = DTid.y * 8 + DTid.x;
    float total = (float)shared_sum / 2080.0;
    output[idx] = float4(total, DTid.x / 7.0, DTid.y / 7.0, 1.0);
}
