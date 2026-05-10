// Exercises SINCOS: c0.x = angle in radians.
// With c0 = (pi/6, 0, 0, 0):  cos(pi/6) = sqrt(3)/2 ≈ 0.866 → R,
//                              sin(pi/6) = 0.5                  → G.
float4 g_angle : register(c0);

float4 main() : COLOR
{
    float s, c;
    sincos(g_angle.x, s, c);
    return float4(c, s, 0.0, 1.0);
}
