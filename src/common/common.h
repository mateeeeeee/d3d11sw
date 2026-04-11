#pragma once

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

#ifdef _WIN32
    #define D3D11SW_COM_UUID(str)          __declspec(uuid(str))
    #define D3D11SW_DEFINE_UUID(type, ...) // nothing, __declspec handles it
#else
    #define D3D11SW_COM_UUID(str)
    #define D3D11SW_DEFINE_UUID(type, ...) DECLARE_UUIDOF_HELPER(type, __VA_ARGS__)
#endif
