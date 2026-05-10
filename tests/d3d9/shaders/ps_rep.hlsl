// Exercises REP/ENDREP: accumulates g_step 3 times.
// Using a literal constant forces FXC to either emit defi+rep or unroll —
// both produce R = 3 × 0.1 = 0.3 with g_step=(0.1, 0, 0, 1).
float4 g_step : register(c0);

float4 main() : COLOR
{
    float4 acc = (float4)0;
    for (int i = 0; i < 3; ++i)
    {
        acc += g_step;
    }
    return acc;
}
