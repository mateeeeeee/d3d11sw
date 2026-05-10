cbuffer Constants : register(b0) { float4 params; };
RWStructuredBuffer<uint> sbuf : register(u0);
RWBuffer<float4> output : register(u1);
groupshared uint shared_acc;

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex)
{
    uint idx = DTid.y * 8 + DTid.x;
    float u = DTid.x / 7.0;
    float v = DTid.y / 7.0;

    // floor/ceil
    float stepped = floor(u * 4.0) / 4.0;
    float rounded = ceil(v * 4.0) / 4.0;

    // sincos
    float s = sin(stepped * 3.14159265);
    float c = cos(rounded * 3.14159265);

    // float comparison + movc
    float r = s >= 0.25 ? s : 0.0;

    // integer math: imad, bitwise
    uint bits = idx * 3 + 7;
    uint popcnt = countbits(bits);
    uint reversed = reversebits(bits);
    uint leading = firstbithigh(bits);

    // structured buffer round-trip
    sbuf[idx] = bits;
    uint readback = sbuf[idx];

    // loop with integer comparison
    uint sum = 0;
    for (uint i = 0; i <= DTid.x; i++)
    {
        sum += i;
    }

    // atomic on shared memory
    if (GI == 0)
    {
        shared_acc = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    InterlockedAdd(shared_acc, 1);
    GroupMemoryBarrierWithGroupSync();

    // combine everything
    float green = (c + 1.0) * 0.5;
    float blue = (float)popcnt / 8.0;
    float alpha_check = (readback == bits) ? 1.0 : 0.0;
    float thread_count = (float)shared_acc / 64.0;

    // mix with cbuffer param
    r = r * params.x + (float)sum / 28.0 * params.y;
    r = saturate(r);

    output[idx] = float4(r, green * thread_count, blue, alpha_check);
}
