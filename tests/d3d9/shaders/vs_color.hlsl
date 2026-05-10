// Pass-through vertex shader with vertex color.
// Used to check that user-defined varyings (COLOR) are interpolated
// correctly between VS output and PS input.

struct VSInput
{
    float3 pos   : POSITION;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 pos   : POSITION;
    float4 color : COLOR;
};

VSOutput main(VSInput inp)
{
    VSOutput o;
    o.pos   = float4(inp.pos, 1.0);
    o.color = inp.color;
    return o;
}
