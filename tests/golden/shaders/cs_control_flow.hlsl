RWBuffer<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint idx = DTid.y * 8 + DTid.x;
    float x = DTid.x / 7.0;
    float y = DTid.y / 7.0;

    float r = 0, g = 0, b = 0;

    // Nested if/else
    if (x < 0.5)
    {
        if (y < 0.5)
        {
            r = 1.0;
        }
        else
        {
            g = 1.0;
        }
    }
    else
    {
        if (y < 0.5)
        {
            b = 1.0;
        }
        else
        {
            r = 0.5;
            g = 0.5;
        }
    }

    // Nested loop with early break
    float accum = 0;
    for (uint i = 0; i < DTid.x + 1; i++)
    {
        for (uint j = 0; j < DTid.y + 1; j++)
        {
            accum += 1.0 / 64.0;
            if (accum > 0.5)
            {
                break;
            }
        }
    }

    float a = saturate(accum);
    output[idx] = float4(r, g, b, a);
}
