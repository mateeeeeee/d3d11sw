#pragma once
#include <cstdint>

struct PostProcessApp
{
    void* device;
    void* context;
    void* swapChain;
    void* backbuffer;
    void* sceneRT;
    void* sceneRTV;
    void* sceneSRV;
    void* postTex;
    void* postUAV;
    void* procTex;
    void* procUAV;
    void* procSRV;
    void* depthTex;
    void* depthDSV;
    void* vs;
    void* ps;
    void* csProc;
    void* csPost;
    void* layout;
    void* vb;
    void* ib;
    void* cbScene;
    void* cbProc;
    void* cbPost;
    void* sampler;
    void* dsState;
    void* rsState;
    uint32_t width;
    uint32_t height;
};

bool PostProcessInit(PostProcessApp& app, void* nsWindow, uint32_t width, uint32_t height);
void PostProcessRenderFrame(PostProcessApp& app, float time);
void PostProcessShutdown(PostProcessApp& app);
