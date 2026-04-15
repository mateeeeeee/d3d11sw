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

[maxvertexcount(4)]
void main(point GSInput input[1], inout TriangleStream<GSOutput> output)
{
    float4 center = input[0].pos;
    float4 col    = input[0].color;
    float  size   = 0.15;

    GSOutput o;
    o.color = col;

    o.pos = center + float4(-size,  size, 0, 0);
    output.Append(o);

    o.pos = center + float4( size,  size, 0, 0);
    output.Append(o);

    o.pos = center + float4(-size, -size, 0, 0);
    output.Append(o);

    o.pos = center + float4( size, -size, 0, 0);
    output.Append(o);

    output.RestartStrip();
}
