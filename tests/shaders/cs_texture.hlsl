Texture2D<float4>   src : register(t0);
RWTexture2D<float4> dst : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    dst[DTid.xy] = src.Load(int3(DTid.x, DTid.y, 0));
}
