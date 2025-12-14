RWBuffer<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    uint sel = idx % 5;
    float r = 0, g = 0, b = 0;

    switch (sel)
    {
        case 0: r = 1.0; break;
        case 1: g = 1.0; break;
        case 2: b = 1.0; break;
        case 3: r = 1.0; g = 1.0; break;
        default: r = 0.5; g = 0.5; b = 0.5; break;
    }

    output[idx] = float4(r, g, b, 1.0);
}
