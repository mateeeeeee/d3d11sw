Texture3D<float4> tex : register(t0);
SamplerState smp : register(s0);
RWTexture2D<float4> dst : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float u = (float)DTid.x / 8.0;
    float v = (float)DTid.y / 8.0;
    float w = (float)DTid.y / 8.0;
    dst[DTid.xy] = tex.SampleLevel(smp, float3(u, v, w), 0);
}
