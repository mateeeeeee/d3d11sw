RWBuffer<float4> output : register(u0);

groupshared uint sharedData[64];

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex)
{
    sharedData[GI] = GI;

    GroupMemoryBarrierWithGroupSync();

    uint mirror = sharedData[63 - GI];

    float val = mirror / 63.0;
    output[GI] = float4(val, 1.0 - val, 0.5, 1.0);
}
