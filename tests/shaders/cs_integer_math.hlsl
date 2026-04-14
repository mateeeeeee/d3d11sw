RWBuffer<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    int x = (int)DTid.x - 4;
    int y = (int)DTid.y - 4;

    int product = x * y;

    uint quotient  = (uint)abs(product + 17) / 7;
    uint remainder = (uint)abs(product + 17) % 7;

    int cmp_eq = (x == y) ? 1 : 0;
    int cmp_lt = (x < y)  ? 1 : 0;
    int cmp_ge = (x >= 0) ? 1 : 0;

    float ftoi_test = (float)x;
    int   itof_test = (int)ftoi_test;
    uint  ftou_test = (uint)abs(x);
    float utof_test = (float)ftou_test;

    float r = saturate((float)abs(product) / 20.0);
    float g = saturate((float)(quotient + remainder) / 10.0);
    float b = saturate((float)(cmp_eq + cmp_lt + cmp_ge) / 3.0);
    float a = saturate(utof_test / 5.0);

    output[idx] = float4(r, g, b, a);
}
