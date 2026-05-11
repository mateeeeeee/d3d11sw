#include "core/common/common.h"
#include "core/common/log.h"
#include "core/common/trace.h"
#include "d3d9/factory/direct3d9.h"

using namespace d3dsw;

extern "C"
{

IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion)
{
    D3DSW_TRACE_CREATE("Direct3DCreate9", "SDKVersion={}", SDKVersion);
    D3DSW_INFO("Direct3DCreate9 called (SDKVersion=%u)", SDKVersion);
    return new D3D9SW();
}

HRESULT WINAPI Direct3DCreate9Ex(UINT SDKVersion, IDirect3D9Ex** ppD3D)
{
    D3DSW_TRACE_CREATE("Direct3DCreate9Ex", "SDKVersion={}", SDKVersion);
    D3DSW_INFO("Direct3DCreate9Ex called (SDKVersion=%u)", SDKVersion);
    if (!ppD3D) 
    { 
        return D3DERR_INVALIDCALL; 
    }
    *ppD3D = new d3dsw::D3D9SW();
    return S_OK;
}

int WINAPI D3DPERF_BeginEvent(D3DCOLOR, LPCWSTR)    { return 0; }
int WINAPI D3DPERF_EndEvent(void)                   { return 0; }
void WINAPI D3DPERF_SetMarker(D3DCOLOR, LPCWSTR)    {}
void WINAPI D3DPERF_SetRegion(D3DCOLOR, LPCWSTR)    {}
BOOL WINAPI D3DPERF_QueryRepeatFrame(void)       { return FALSE; }
void WINAPI D3DPERF_SetOptions(DWORD)               {}
DWORD WINAPI D3DPERF_GetStatus(void)                { return 0; }

}
