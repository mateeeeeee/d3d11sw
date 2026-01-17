Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);
RWTexture2D<float4> dst : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float lod = (float)DTid.y / 2.0;
    dst[DTid.xy] = tex.SampleLevel(smp, float2(0.5, 0.5), lod);
}
