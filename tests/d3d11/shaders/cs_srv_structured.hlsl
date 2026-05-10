StructuredBuffer<uint> input : register(t0);
RWBuffer<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    uint v = input[idx];
    float norm = v / 255.0;
    output[idx] = float4(norm, 1.0 - norm, (float)(idx % 8) / 7.0, 1.0);
}
