RWStructuredBuffer<uint> sbuf : register(u0);
RWBuffer<float4> output : register(u1);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    sbuf[idx] = idx * 4 + 1;
    uint v = sbuf[idx];
    output[idx] = float4(v / 255.0, DTid.x / 7.0, DTid.y / 7.0, 1.0);
}
