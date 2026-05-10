Texture2D<float>        depthTex : register(t0);
SamplerComparisonState  cmpSmp   : register(s0);
RWTexture2D<float4>     dst      : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // Sample at texel center: (x+0.5)/8 so we land exactly on one texel
    float2 uv = float2((DTid.x + 0.5) / 8.0, (DTid.y + 0.5) / 8.0);
    // depthTex contains x/8 per column; compare against 0.5 with LESS
    // columns 0-3 pass (depth < 0.5), columns 4-7 fail
    float r = depthTex.SampleCmpLevelZero(cmpSmp, uv, 0.5);
    dst[DTid.xy] = float4(r, r, r, 1.0);
}
