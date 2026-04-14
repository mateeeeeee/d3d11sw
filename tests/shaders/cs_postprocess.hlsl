Texture2D<float4> input : register(t0);
RWTexture2D<float4> output : register(u0);

cbuffer CB : register(b0)
{
    float screenWidth;
    float screenHeight;
    float time;
    float pad;
};

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    float2 center = float2(screenWidth * 0.5, screenHeight * 0.5);
    float dx = (float)dtid.x - center.x;
    float dy = (float)dtid.y - center.y;
    float dist = sqrt(dx * dx + dy * dy) / (screenWidth * 0.5);

    int offset = (int)(dist * 12.0 + 2.0);
    float r = input.Load(int3(dtid.x + offset, dtid.y + offset / 2, 0)).r;
    float g = input.Load(int3(dtid.xy, 0)).g;
    float b = input.Load(int3(dtid.x - offset, dtid.y - offset / 2, 0)).b;

    output[dtid.xy] = float4(r, g, b, 1.0);
}
