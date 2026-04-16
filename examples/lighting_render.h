#pragma once
#include <cstdint>

struct LightingApp
{
    void* device;
    void* context;
    void* swapChain;
    void* rtv;
    void* dsv;
    void* vs;
    void* psLighting;
    void* psSolid;
    void* layout;
    void* vb;
    void* ib;
    void* cb;
    void* dsState;
    void* rsState;
    void* dsTex;
    void* backbuffer;
    uint32_t width;
    uint32_t height;
};

bool LightingInit(LightingApp& app, void* nsWindow, uint32_t width, uint32_t height);
void LightingRenderFrame(LightingApp& app, float time);
void LightingShutdown(LightingApp& app);
