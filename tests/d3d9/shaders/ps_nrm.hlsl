// Exercises NRM (normalize): reads a vector from c0.xyz, normalizes it, outputs as RGB.
// With c0 = (1, 1, 0, 0):  normalize(1,1,0) = (1/sqrt2, 1/sqrt2, 0) ≈ (0.707, 0.707, 0).
float4 g_vec : register(c0);

float4 main() : COLOR
{
    return float4(normalize(g_vec.xyz), 1.0);
}
