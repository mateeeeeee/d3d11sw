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

[maxvertexcount(6)]
void main(triangle GSInput input[3], inout LineStream<GSOutput> output)
{
    for (int i = 0; i < 3; i++)
    {
        int j = (i + 1) % 3;

        GSOutput o;

        o.color = input[i].color;
        o.pos   = input[i].pos;
        output.Append(o);

        o.color = input[j].color;
        o.pos   = input[j].pos;
        output.Append(o);

        output.RestartStrip();
    }
}
