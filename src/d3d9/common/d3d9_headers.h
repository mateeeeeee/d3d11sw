#pragma once

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #include <windows.h>
    #include <d3d9.h>

    // Windows SDK lacks D3DSHADER_PARAM_DSTMOD_TYPE (only existed in d3d8types.h)
    typedef DWORD D3DSHADER_PARAM_DSTMOD_TYPE;
#else
    #define COM_NO_WINDOWS_H
    #include <windows.h>
    #include <d3d9.h>
#endif
