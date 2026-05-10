Texture2DMS<float4, 4> msTexture : register(t0);

struct PSInput
{
    float4 pos : SV_Position;
};

float4 main(PSInput inp) : SV_Target
{
    int2 coord = int2(inp.pos.xy);
    float4 sum = float4(0, 0, 0, 0);
    sum += msTexture.Load(coord, 0);
    sum += msTexture.Load(coord, 1);
    sum += msTexture.Load(coord, 2);
    sum += msTexture.Load(coord, 3);
    return sum * 0.25;
}
