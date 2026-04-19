struct PSInput
{
    float4 pos : SV_Position;
    uint sampleIdx : SV_SampleIndex;
};

float4 main(PSInput inp) : SV_Target
{
    float t = (float)inp.sampleIdx / 3.0;
    return float4(1.0 - t, t, 0.0, 1.0);
}
