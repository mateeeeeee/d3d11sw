#pragma once
#include <cstdint>

struct InstancedApp
{
    void* device;
    void* context;
    void* swapChain;
    void* rtv;
    void* dsv;
    void* vs;
    void* ps;
    void* layout;
    void* vb;
    void* ib;
    void* cb;
    void* srv;
    void* sampler;
    void* dsState;
    void* rsState;
    void* dsTex;
    void* backbuffer;
    uint32_t width;
    uint32_t height;
};

bool InstancedInit(InstancedApp& app, void* nsWindow, uint32_t width, uint32_t height);
void InstancedRenderFrame(InstancedApp& app, float time);
void InstancedShutdown(InstancedApp& app);
