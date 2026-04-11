#pragma once
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <functional>

inline LRESULT CALLBACK Win32WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

inline HWND Win32CreateWindow(const char* title, uint32_t width, uint32_t height)
{
    WNDCLASSEXA wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = Win32WndProc;
    wc.hInstance      = GetModuleHandleA(nullptr);
    wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    wc.lpszClassName  = "d3d11sw";
    RegisterClassExA(&wc);

    RECT rc = {0, 0, (LONG)width, (LONG)height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExA(
        0, "d3d11sw", title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, wc.hInstance, nullptr);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return hwnd;
}

inline void Win32RunLoop(std::function<void()> tick = nullptr, std::function<void(WPARAM)> onKey = nullptr)
{
    MSG msg{};
    if (tick)
    {
        while (true)
        {
            while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT) { return; }
                if (msg.message == WM_KEYDOWN && onKey) { onKey(msg.wParam); }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
            tick();
        }
    }
    else
    {
        while (GetMessageA(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
}

#endif
