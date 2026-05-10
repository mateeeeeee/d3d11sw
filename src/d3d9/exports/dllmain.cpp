#include "core/common/common.h"
#include "core/common/log.h"

using namespace d3dsw;

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        D3DSW_INFO("d3dsw d3d9.dll loaded");
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        D3DSW_INFO("d3dsw d3d9.dll unloaded");
        break;
    }
    return TRUE;
}

#endif
