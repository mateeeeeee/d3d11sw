#pragma once

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <d3d11_4.h>
    #include <dxgi1_2.h>
#else
    #define COM_NO_WINDOWS_H
    #include <native-windows/windows.h>
    #include <directx-headers/d3d11_4.h>
    #include <directx-headers/dxgi1_2.h>
#endif

#include <atomic>
