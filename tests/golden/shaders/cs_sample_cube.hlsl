TextureCube<float4> tex : register(t0);
SamplerState smp : register(s0);
RWTexture2D<float4> dst : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float u = (float)DTid.x / 4.0 - 1.0;
    float v = (float)DTid.y / 4.0 - 1.0;
    dst[DTid.xy] = tex.SampleLevel(smp, float3(1.0, u, v), 0);
}
