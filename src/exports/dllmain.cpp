#include "common/common.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) 
    {
    case DLL_PROCESS_ATTACH:
        MARS_LOG("MARS d3d11.dll loaded");
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        MARS_LOG("MARS d3d11.dll unloaded");
        break;
    }
    return TRUE;
}
