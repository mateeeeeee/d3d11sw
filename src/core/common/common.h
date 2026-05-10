#pragma once

// Basic platform types + Windows typedefs (UINT, HMODULE, HRESULT, etc.)
// Core uses these throughout. On non-Windows, this is satisfied by the
// third_party/native-windows shims.
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #define COM_NO_WINDOWS_H
    #include <windows.h>
#endif

#ifdef _WIN32
    #define D3DSW_COM_UUID(str)          __declspec(uuid(str))
    #define D3DSW_DEFINE_UUID(type, ...) // nothing, __declspec handles it
#else
    #define D3DSW_COM_UUID(str)
    #define D3DSW_DEFINE_UUID(type, ...) DECLARE_UUIDOF_HELPER(type, __VA_ARGS__)
#endif
