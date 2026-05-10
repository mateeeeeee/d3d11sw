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
    float4 center = (input[0].pos + input[1].pos + input[2].pos) / 3.0;
    for (int i = 0; i < 3; i++)
    {
        GSOutput o;
        o.pos   = lerp(center, input[i].pos, 0.5);
        o.color = input[i].color;
        output.Append(o);
    }
    output.RestartStrip();
}
