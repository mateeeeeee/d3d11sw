#include "common/common.h"
#include "common/log.h"

using namespace d3d11sw;

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        D3D11SW_INFO("d3d11sw d3d11.dll loaded");
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        D3D11SW_INFO("d3d11sw d3d11.dll unloaded");
        break;
    }
    return TRUE;
}

#else

__attribute__((constructor)) static void on_load()   { D3D11SW_INFO("d3d11sw loaded"); }
__attribute__((destructor))  static void on_unload() { D3D11SW_INFO("d3d11sw unloaded"); }

#endif
