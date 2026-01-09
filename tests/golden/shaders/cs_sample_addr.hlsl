Texture2D<float4>   src : register(t0);
SamplerState        smp : register(s0);
RWTexture2D<float4> dst : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float u = (float)DTid.x / 8.0 * 3.0 - 1.0;
    float v = (float)DTid.y / 8.0 * 3.0 - 1.0;
    dst[DTid.xy] = src.SampleLevel(smp, float2(u, v), 0);
}
