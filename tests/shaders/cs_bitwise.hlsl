RWBuffer<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    uint val = DTid.x + DTid.y * 8;

    uint a = val;
    uint b = val ^ 0xFF;

    uint and_result  = a & b;
    uint or_result   = a | b;
    uint xor_result  = a ^ b;
    uint not_result  = ~a;
    uint shl_result  = a << 2;
    uint shr_result  = not_result >> 4;

    uint bits = countbits(val);
    uint rev  = reversebits(val);

    float r = (and_result & 0xFF) / 255.0;
    float g = (or_result  & 0xFF) / 255.0;
    float b_ch = (xor_result  & 0xFF) / 255.0;
    float a_ch = ((shl_result + shr_result + bits + (rev >> 24)) & 0xFF) / 255.0;

    output[idx] = float4(r, g, b_ch, a_ch);
}
