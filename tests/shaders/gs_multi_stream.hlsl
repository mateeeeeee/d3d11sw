struct GSInput
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
};

struct Stream0Vert
{
    float4 pos : SV_Position;
};

struct Stream1Vert
{
    float4 color : COLOR;
};

[maxvertexcount(6)]
void main(triangle GSInput input[3],
          inout PointStream<Stream0Vert> stream0,
          inout PointStream<Stream1Vert> stream1)
{
    for (int i = 0; i < 3; i++)
    {
        Stream0Vert v0;
        v0.pos = input[i].pos;
        stream0.Append(v0);

        Stream1Vert v1;
        v1.color = input[i].color;
        stream1.Append(v1);
    }
}
