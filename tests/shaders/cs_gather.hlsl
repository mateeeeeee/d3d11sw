Texture2D<float4>   src : register(t0);
SamplerState        smp : register(s0);
RWTexture2D<float4> dst : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // Sample at texel center; GatherRed returns R channel from 2x2 footprint:
    //   x = texel(i, j+1).r,  y = texel(i+1, j+1).r
    //   z = texel(i+1, j).r,  w = texel(i, j).r
    float2 uv = float2((DTid.x + 0.5) / 8.0, (DTid.y + 0.5) / 8.0);
    dst[DTid.xy] = src.GatherRed(smp, uv);
}
