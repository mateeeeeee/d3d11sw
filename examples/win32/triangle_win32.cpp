#ifdef _WIN32
#include "win32_window.h"
#include "triangle_render.h"

int main()
{
    HWND hwnd = Win32CreateWindow("d3d11sw triangle", 640, 480);
    RunTriangle((void*)hwnd, 640, 480);
    Win32RunLoop();
    return 0;
}
#endif
