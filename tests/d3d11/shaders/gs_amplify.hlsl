struct GSInput
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
};

struct GSOutput
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
};

[maxvertexcount(9)]
void main(triangle GSInput input[3], inout TriangleStream<GSOutput> output)
{
    float4 offsets[3];
    offsets[0] = float4(-0.3, 0.15, 0, 0);
    offsets[1] = float4( 0.3, 0.15, 0, 0);
    offsets[2] = float4( 0.0,-0.25, 0, 0);

    float4 tints[3];
    tints[0] = float4(1.0, 0.3, 0.3, 1.0);
    tints[1] = float4(0.3, 1.0, 0.3, 1.0);
    tints[2] = float4(0.3, 0.3, 1.0, 1.0);

    for (int c = 0; c < 3; c++)
    {
        for (int i = 0; i < 3; i++)
        {
            GSOutput o;
            o.pos   = input[i].pos * 0.35 + offsets[c];
            o.pos.w = 1.0;
            o.color = tints[c];
            output.Append(o);
        }
        output.RestartStrip();
    }
}
