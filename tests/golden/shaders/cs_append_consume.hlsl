AppendStructuredBuffer<uint> appendBuf : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint idx = dtid.y * 8 + dtid.x;
    appendBuf.Append(idx);
}
