cbuffer Constants : register(b0) { float4 color; };
RWBuffer<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    float u = DTid.x / 7.0;
    output[idx] = float4(color.rgb * u, 1.0);
}
