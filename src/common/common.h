#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

//for now use Windows SDK headers, later write our own to avoid dependency
#include <windows.h>
#include <d3d11_4.h>
#include <dxgi1_2.h>
#include <atomic>
