#pragma once
#include <cstdint>

struct TessApp
{
    void* device;
    void* context;
    void* swapChain;
    void* rtv;
    void* vs;
    void* hs;
    void* ds;
    void* ps;
    void* layout;
    void* vb;
    void* ib;
    void* cb;
    void* rsState;
    void* rsSolid;
    void* backbuffer;
    uint32_t width;
    uint32_t height;
    uint32_t indexCount;
    float camYaw;
    float camPitch;
    float camDist;
    bool wireframe;
};

bool TessInit(TessApp& app, void* nsWindow, uint32_t width, uint32_t height);
void TessRenderFrame(TessApp& app, float dt);
void TessShutdown(TessApp& app);
