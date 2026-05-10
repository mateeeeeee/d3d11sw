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

    // Modern Windows SDK lacks D3DSHADER_PARAM_DSTMOD_TYPE (only existed in d3d8types.h)
    #ifndef D3DSP_DSTMOD_SHIFT
    #define D3DSP_DSTMOD_SHIFT 20
    #define D3DSP_DSTMOD_MASK  (0xF << D3DSP_DSTMOD_SHIFT)
    #endif

    #ifndef D3DSPDM_NONE
    typedef enum _D3DSHADER_PARAM_DSTMOD_TYPE {
        D3DSPDM_NONE             = 0 << D3DSP_DSTMOD_SHIFT,
        D3DSPDM_SATURATE         = 1 << D3DSP_DSTMOD_SHIFT,
        D3DSPDM_PARTIALPRECISION = 2 << D3DSP_DSTMOD_SHIFT,
        D3DSPDM_MSAMPCENTROID    = 4 << D3DSP_DSTMOD_SHIFT,
        D3DSPDM_FORCE_DWORD      = 0x7FFFFFFF
    } D3DSHADER_PARAM_DSTMOD_TYPE;
    #endif
#else
    #define COM_NO_WINDOWS_H
    #include <windows.h>
    #include <d3d9.h>
#endif
