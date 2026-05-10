#pragma once
#include <cstdint>

struct D3D9SceneApp
{
    void* d3d9;
    void* device;
    void* vbCube;
    void* ibCube;
    void* vbFloor;
    void* texCube;
    void* texFloor;
    void* depthSurface;
    uint32_t width;
    uint32_t height;
};

bool D3D9SceneInit(D3D9SceneApp& app, void* nsWindow, uint32_t width, uint32_t height);
void D3D9SceneRenderFrame(D3D9SceneApp& app, float time);
void D3D9SceneShutdown(D3D9SceneApp& app);
