#pragma once

// Central D3D11 platform header bundle. Include this from D3D11-side headers
// that need D3D11/DXGI types. Core/ never includes this.
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #include <windows.h>
    #include <d3d11_4.h>
    #include <dxgi1_2.h>
#else
    #define COM_NO_WINDOWS_H
    #include <windows.h>
    #include <d3d11_4.h>
    #include <dxgi1_2.h>
#endif
