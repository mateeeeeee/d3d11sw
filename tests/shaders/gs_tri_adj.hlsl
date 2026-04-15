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

[maxvertexcount(12)]
void main(triangleadj GSInput input[6], inout TriangleStream<GSOutput> output)
{
    float size = 0.1;
    GSOutput o;

    // Emit the original triangle (v0, v2, v4) in dark gray
    o.color = float4(0.3, 0.3, 0.3, 1.0);
    o.pos = input[0].pos; output.Append(o);
    o.pos = input[4].pos; output.Append(o);
    o.pos = input[2].pos; output.Append(o);
    output.RestartStrip();

    // Emit small triangles at each adjacent vertex position (v1, v3, v5)
    for (int a = 0; a < 3; a++)
    {
        int adjIdx = a * 2 + 1;
        float4 center = input[adjIdx].pos;
        float4 col    = input[adjIdx].color;

        o.color = col;
        o.pos = center + float4(0, size, 0, 0);     output.Append(o);
        o.pos = center + float4(size, -size, 0, 0);  output.Append(o);
        o.pos = center + float4(-size, -size, 0, 0); output.Append(o);
        output.RestartStrip();
    }
}
