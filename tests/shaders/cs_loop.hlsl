RWBuffer<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    float sum = 0.0;
    for (uint i = 0; i <= DTid.x; i++) { sum += 1.0 / 8.0; }
    float row = DTid.y / 7.0;
    output[idx] = float4(sum, row, 0.0, 1.0);
}
