RWBuffer<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    float u = DTid.x / 7.0;
    float v = DTid.y / 7.0;

    float s = sin(u * 3.14159265);
    float c = cos(v * 3.14159265);

    float r = s >= 0.5 ? 1.0 : s;
    float g = (c + 1.0) * 0.5;

    output[idx] = float4(r, g, u * v, 1.0);
}
