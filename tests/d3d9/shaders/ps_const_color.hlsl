// Constant-color pixel shader: returns g_color from a single constant register.
// Exercises the c#-bank → cbuffer plumbing from the device's
// SetPixelShaderConstantF state to the shader at sample/draw time.

float4 g_color : register(c0);

float4 main() : COLOR
{
    return g_color;
}
