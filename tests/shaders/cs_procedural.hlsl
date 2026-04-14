RWTexture2D<float4> output : register(u0);

cbuffer CB : register(b0)
{
    float time;
    float texWidth;
    float texHeight;
    float pad;
};

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    float2 uv = float2(dtid.xy) / float2(texWidth, texHeight);
    float2 p = uv * 4.0 - 2.0;

    float a = sin(p.x * 3.0 + time * 1.2) + sin(p.y * 2.0 - time * 0.8);
    float b = sin(p.x * 1.5 - time * 0.6 + p.y * 2.5) + sin(sqrt(p.x * p.x + p.y * p.y) * 4.0 - time * 1.5);
    float c = (a + b) * 0.25 + 0.5;

    float r = sin(c * 6.28 + 0.0) * 0.5 + 0.5;
    float g = sin(c * 6.28 + 2.09) * 0.5 + 0.5;
    float bl = sin(c * 6.28 + 4.19) * 0.5 + 0.5;

    output[dtid.xy] = float4(r, g, bl, 1.0);
}
