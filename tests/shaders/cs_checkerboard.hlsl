RWBuffer<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    uint check = (DTid.x ^ DTid.y) & 1;
    float c = (float)check;
    output[idx] = float4(c, c, c, 1.0);
}
