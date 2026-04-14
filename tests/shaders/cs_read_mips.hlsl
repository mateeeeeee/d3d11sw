Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);
RWTexture2D<float4> dst : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint x = DTid.x;
    uint y = DTid.y;

    // Layout:
    //   cols 0-3, rows 0-3: mip1 (4x4)
    //   cols 4-5, rows 0-1: mip2 (2x2)
    //   col  4,   row  2:   mip3 (1x1)
    //   rest: mip0 pixels

    float4 c = float4(0, 0, 0, 1);
    if (x < 4 && y < 4)
    {
        c = tex.SampleLevel(smp, float2((float)x / 4.0 + 0.125, (float)y / 4.0 + 0.125), 1);
    }
    else if (x >= 4 && x < 6 && y < 2)
    {
        c = tex.SampleLevel(smp, float2((float)(x - 4) / 2.0 + 0.25, (float)y / 2.0 + 0.25), 2);
    }
    else if (x == 4 && y == 2)
    {
        c = tex.SampleLevel(smp, float2(0.5, 0.5), 3);
    }
    else
    {
        c = tex.SampleLevel(smp, float2((float)x / 8.0 + 0.0625, (float)y / 8.0 + 0.0625), 0);
    }

    dst[DTid.xy] = c;
}
