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
void main(triangle GSInput input[3], inout TriangleStream<GSOutput> output)
{
    float4 p0 = input[0].pos;
    float4 p1 = input[1].pos;
    float4 p2 = input[2].pos;
    float4 c0 = input[0].color;
    float4 c1 = input[1].color;
    float4 c2 = input[2].color;

    float4 m01 = (p0 + p1) * 0.5;
    float4 m12 = (p1 + p2) * 0.5;
    float4 m20 = (p2 + p0) * 0.5;
    float4 cm01 = (c0 + c1) * 0.5;
    float4 cm12 = (c1 + c2) * 0.5;
    float4 cm20 = (c2 + c0) * 0.5;

    GSOutput o;

    o.pos = p0;   o.color = c0;   output.Append(o);
    o.pos = m01;  o.color = cm01; output.Append(o);
    o.pos = m20;  o.color = cm20; output.Append(o);
    output.RestartStrip();

    o.pos = m01;  o.color = cm01; output.Append(o);
    o.pos = p1;   o.color = c1;   output.Append(o);
    o.pos = m12;  o.color = cm12; output.Append(o);
    output.RestartStrip();

    o.pos = m20;  o.color = cm20; output.Append(o);
    o.pos = m12;  o.color = cm12; output.Append(o);
    o.pos = p2;   o.color = c2;   output.Append(o);
    output.RestartStrip();

    o.pos = m01;  o.color = cm01; output.Append(o);
    o.pos = m12;  o.color = cm12; output.Append(o);
    o.pos = m20;  o.color = cm20; output.Append(o);
    output.RestartStrip();
}
