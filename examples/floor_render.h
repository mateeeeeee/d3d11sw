#pragma once
#include <cstdint>

struct FloorApp
{
    void* device;
    void* context;
    void* swapChain;
    void* backbuffer;
    void* depthTex;
    void* depthDSV;
    void* vs;
    void* psAutoLod;
    void* psLod0;
    void* layout;
    void* vb;
    void* cb;
    void* tex;
    void* srv;
    void* sampler;
    void* dsState;
    void* rsState;
    uint32_t width;
    uint32_t height;
    bool autoLod;
};

bool FloorInit(FloorApp& app, void* nsWindow, uint32_t width, uint32_t height);
void FloorRenderFrame(FloorApp& app, float time);
void FloorShutdown(FloorApp& app);
void FloorToggleLod(FloorApp& app);
