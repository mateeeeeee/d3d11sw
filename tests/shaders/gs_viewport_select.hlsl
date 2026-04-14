struct GSInput
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
};

struct GSOutput
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
    uint   vpIdx : SV_ViewportArrayIndex;
};

[maxvertexcount(6)]
void main(triangle GSInput input[3], inout TriangleStream<GSOutput> output)
{
    for (int i = 0; i < 3; i++)
    {
        GSOutput o;
        o.pos   = input[i].pos;
        o.color = float4(1.0, 0.3, 0.3, 1.0);
        o.vpIdx = 0;
        output.Append(o);
    }
    output.RestartStrip();

    for (int i = 0; i < 3; i++)
    {
        GSOutput o;
        o.pos   = input[i].pos;
        o.color = float4(0.3, 0.3, 1.0, 1.0);
        o.vpIdx = 1;
        output.Append(o);
    }
    output.RestartStrip();
}
