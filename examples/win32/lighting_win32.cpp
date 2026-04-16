#ifdef _WIN32
#include "win32_window.h"
#include "lighting_render.h"
#include <cstring>

int main()
{
    const uint32_t W = 800, H = 600;
    HWND hwnd = Win32CreateWindow("d3d11sw lighting", W, H);

    LightingApp app;
    memset(&app, 0, sizeof(app));
    if (!LightingInit(app, (void*)hwnd, W, H))
    {
        return 1;
    }

    LARGE_INTEGER freq, start;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    Win32RunLoop([&]()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float t = (float)(now.QuadPart - start.QuadPart) / (float)freq.QuadPart;
        LightingRenderFrame(app, t);
    });

    LightingShutdown(app);
    return 0;
}
#endif
