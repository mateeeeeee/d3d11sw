// Exercises POW: c0.x = base, c0.y = exponent.
// FXC expands pow(abs(x), y) to ABS + LOG + MUL + EXP in SM3, exercising
// all four opcodes. With c0 = (0.5, 2, 0, 0): pow(0.5, 2) = 0.25.
float4 g_val : register(c0);

float4 main() : COLOR
{
    float p = pow(abs(g_val.x), g_val.y);
    return float4(p, p, p, 1.0);
}
