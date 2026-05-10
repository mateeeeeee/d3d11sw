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

[maxvertexcount(3)]
void main(triangle GSInput input[3], inout TriangleStream<GSOutput> output)
{
    for (int i = 0; i < 3; i++)
    {
        GSOutput o;
        o.pos   = input[i].pos;
        o.color = input[i].color;
        output.Append(o);
    }
    output.RestartStrip();
}
