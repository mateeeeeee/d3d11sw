#define COM_NO_WINDOWS_H
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "d3d9_scene_render.h"

struct CubeVertex
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

struct FloorVertex
{
    float x, y, z;
    float nx, ny, nz;
    DWORD color;
    float u, v;
};

static constexpr DWORD FVF_CUBE  = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;
static constexpr DWORD FVF_FLOOR = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;

static void MakeIdentity(D3DMATRIX& m)
{
    memset(&m, 0, sizeof(m));
    m._11 = m._22 = m._33 = m._44 = 1.f;
}

static void MakeRotationY(float angle, D3DMATRIX& m)
{
    MakeIdentity(m);
    float c = cosf(angle), s = sinf(angle);
    m._11 = c;  m._13 = s;
    m._31 = -s; m._33 = c;
}

static void MakeRotationX(float angle, D3DMATRIX& m)
{
    MakeIdentity(m);
    float c = cosf(angle), s = sinf(angle);
    m._22 = c;  m._23 = -s;
    m._32 = s;  m._33 = c;
}

static void MatMul(const D3DMATRIX& a, const D3DMATRIX& b, D3DMATRIX& out)
{
    const float* A = &a._11;
    const float* B = &b._11;
    float* O = &out._11;
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            float sum = 0.f;
            for (int k = 0; k < 4; ++k)
            {
                sum += A[r * 4 + k] * B[k * 4 + c];
            }
            O[r * 4 + c] = sum;
        }
    }
}

static void MakeLookAtLH(float eyeX, float eyeY, float eyeZ,
                          float atX, float atY, float atZ,
                          float upX, float upY, float upZ, D3DMATRIX& m)
{
    float fx = atX - eyeX, fy = atY - eyeY, fz = atZ - eyeZ;
    float fl = sqrtf(fx*fx + fy*fy + fz*fz);
    fx /= fl; fy /= fl; fz /= fl;

    float sx = upY*fz - upZ*fy, sy = upZ*fx - upX*fz, sz = upX*fy - upY*fx;
    float sl = sqrtf(sx*sx + sy*sy + sz*sz);
    sx /= sl; sy /= sl; sz /= sl;

    float ux = fy*sz - fz*sy, uy = fz*sx - fx*sz, uz = fx*sy - fy*sx;

    MakeIdentity(m);
    m._11 = sx;  m._12 = ux;  m._13 = fx;
    m._21 = sy;  m._22 = uy;  m._23 = fy;
    m._31 = sz;  m._32 = uz;  m._33 = fz;
    m._41 = -(sx*eyeX + sy*eyeY + sz*eyeZ);
    m._42 = -(ux*eyeX + uy*eyeY + uz*eyeZ);
    m._43 = -(fx*eyeX + fy*eyeY + fz*eyeZ);
}

static void MakePerspectiveFovLH(float fovY, float aspect, float zn, float zf, D3DMATRIX& m)
{
    memset(&m, 0, sizeof(m));
    float h = 1.f / tanf(fovY * 0.5f);
    m._11 = h / aspect;
    m._22 = h;
    m._33 = zf / (zf - zn);
    m._34 = 1.f;
    m._43 = -(zn * zf) / (zf - zn);
}

static void GenerateCheckerboard(uint8_t* data, uint32_t w, uint32_t h, uint32_t pitch,
                                  uint8_t r0, uint8_t g0, uint8_t b0,
                                  uint8_t r1, uint8_t g1, uint8_t b1, uint32_t cellSize)
{
    for (uint32_t y = 0; y < h; ++y)
    {
        uint8_t* row = data + y * pitch;
        for (uint32_t x = 0; x < w; ++x)
        {
            bool dark = ((x / cellSize) + (y / cellSize)) & 1;
            row[x * 4 + 0] = dark ? b1 : b0;
            row[x * 4 + 1] = dark ? g1 : g0;
            row[x * 4 + 2] = dark ? r1 : r0;
            row[x * 4 + 3] = 0xFF;
        }
    }
}

bool D3D9SceneInit(D3D9SceneApp& app, void* nsWindow, uint32_t width, uint32_t height)
{
    app.width  = width;
    app.height = height;

    IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d9)
    {
        fprintf(stderr, "Direct3DCreate9 failed\n");
        return false;
    }
    app.d3d9 = d3d9;

    D3DPRESENT_PARAMETERS pp{};
    pp.BackBufferWidth  = width;
    pp.BackBufferHeight = height;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferCount  = 1;
    pp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow    = (HWND)nsWindow;
    pp.Windowed         = TRUE;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_D24S8;

    IDirect3DDevice9* device = nullptr;
    HRESULT hr = d3d9->CreateDevice(0, D3DDEVTYPE_HAL, (HWND)nsWindow,
                                    D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                    &pp, &device);
    if (FAILED(hr))
    {
        fprintf(stderr, "CreateDevice failed: 0x%08X\n", (unsigned)hr);
        return false;
    }
    app.device = device;

    // Cube: 24 vertices (4 per face, unique normals + UVs), 36 indices
    CubeVertex cubeVerts[] = {
        // Front (+Z)
        {-1, -1,  1,  0, 0, 1,  0, 1}, { 1, -1,  1,  0, 0, 1,  1, 1},
        { 1,  1,  1,  0, 0, 1,  1, 0}, {-1,  1,  1,  0, 0, 1,  0, 0},
        // Back (-Z)
        { 1, -1, -1,  0, 0,-1,  0, 1}, {-1, -1, -1,  0, 0,-1,  1, 1},
        {-1,  1, -1,  0, 0,-1,  1, 0}, { 1,  1, -1,  0, 0,-1,  0, 0},
        // Right (+X)
        { 1, -1,  1,  1, 0, 0,  0, 1}, { 1, -1, -1,  1, 0, 0,  1, 1},
        { 1,  1, -1,  1, 0, 0,  1, 0}, { 1,  1,  1,  1, 0, 0,  0, 0},
        // Left (-X)
        {-1, -1, -1, -1, 0, 0,  0, 1}, {-1, -1,  1, -1, 0, 0,  1, 1},
        {-1,  1,  1, -1, 0, 0,  1, 0}, {-1,  1, -1, -1, 0, 0,  0, 0},
        // Top (+Y)
        {-1,  1,  1,  0, 1, 0,  0, 1}, { 1,  1,  1,  0, 1, 0,  1, 1},
        { 1,  1, -1,  0, 1, 0,  1, 0}, {-1,  1, -1,  0, 1, 0,  0, 0},
        // Bottom (-Y)
        {-1, -1, -1,  0,-1, 0,  0, 1}, { 1, -1, -1,  0,-1, 0,  1, 1},
        { 1, -1,  1,  0,-1, 0,  1, 0}, {-1, -1,  1,  0,-1, 0,  0, 0},
    };

    uint16_t cubeIdx[] = {
         0,  2,  1,   0,  3,  2,
         4,  6,  5,   4,  7,  6,
         8, 10,  9,   8, 11, 10,
        12, 14, 13,  12, 15, 14,
        16, 18, 17,  16, 19, 18,
        20, 22, 21,  20, 23, 22,
    };

    IDirect3DVertexBuffer9* vbCube = nullptr;
    device->CreateVertexBuffer(sizeof(cubeVerts), 0, FVF_CUBE, D3DPOOL_DEFAULT, &vbCube, nullptr);
    void* ptr = nullptr;
    vbCube->Lock(0, sizeof(cubeVerts), &ptr, 0);
    memcpy(ptr, cubeVerts, sizeof(cubeVerts));
    vbCube->Unlock();
    app.vbCube = vbCube;

    IDirect3DIndexBuffer9* ibCube = nullptr;
    device->CreateIndexBuffer(sizeof(cubeIdx), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ibCube, nullptr);
    ibCube->Lock(0, sizeof(cubeIdx), &ptr, 0);
    memcpy(ptr, cubeIdx, sizeof(cubeIdx));
    ibCube->Unlock();
    app.ibCube = ibCube;

    // Floor: large quad at y=-1.5
    float floorSize = 8.f;
    float uvScale = 4.f;
    DWORD white = 0xFFFFFFFF;
    FloorVertex floorVerts[] = {
        {-floorSize, -1.5f, -floorSize,  0, 1, 0, white,  0,       uvScale},
        { floorSize, -1.5f, -floorSize,  0, 1, 0, white,  uvScale, uvScale},
        { floorSize, -1.5f,  floorSize,  0, 1, 0, white,  uvScale, 0},
        {-floorSize, -1.5f,  floorSize,  0, 1, 0, white,  0,       0},
    };

    IDirect3DVertexBuffer9* vbFloor = nullptr;
    device->CreateVertexBuffer(sizeof(floorVerts), 0, FVF_FLOOR, D3DPOOL_DEFAULT, &vbFloor, nullptr);
    vbFloor->Lock(0, sizeof(floorVerts), &ptr, 0);
    memcpy(ptr, floorVerts, sizeof(floorVerts));
    vbFloor->Unlock();
    app.vbFloor = vbFloor;

    // Cube texture: orange/dark orange checkerboard 32x32
    IDirect3DTexture9* texCube = nullptr;
    device->CreateTexture(32, 32, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texCube, nullptr);
    D3DLOCKED_RECT lr;
    texCube->LockRect(0, &lr, nullptr, 0);
    GenerateCheckerboard((uint8_t*)lr.pBits, 32, 32, lr.Pitch,
                         0xFF, 0x99, 0x33,    // orange
                         0xCC, 0x66, 0x00,    // dark orange
                         4);
    texCube->UnlockRect(0);
    app.texCube = texCube;

    // Floor texture: white/gray checkerboard 64x64
    IDirect3DTexture9* texFloor = nullptr;
    device->CreateTexture(64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texFloor, nullptr);
    texFloor->LockRect(0, &lr, nullptr, 0);
    GenerateCheckerboard((uint8_t*)lr.pBits, 64, 64, lr.Pitch,
                         0xDD, 0xDD, 0xDD,    // light gray
                         0x55, 0x55, 0x55,    // dark gray
                         8);
    texFloor->UnlockRect(0);
    app.texFloor = texFloor;

    // Render states that persist across frames
    device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    device->SetRenderState(D3DRS_LIGHTING, TRUE);
    device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_AMBIENT, 0x00606060);

    device->SetRenderState(D3DRS_FOGENABLE, TRUE);
    device->SetRenderState(D3DRS_FOGCOLOR, 0xFF334455);
    device->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
    float fogStart = 2.f, fogEnd = 8.f;
    device->SetRenderState(D3DRS_FOGSTART, *(DWORD*)&fogStart);
    device->SetRenderState(D3DRS_FOGEND, *(DWORD*)&fogEnd);

    // Sampler state
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

    // Texture stage: MODULATE texture with diffuse lighting
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

    return true;
}

void D3D9SceneRenderFrame(D3D9SceneApp& app, float time)
{
    IDirect3DDevice9* device = (IDirect3DDevice9*)app.device;

    device->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                  0xFF334455, 1.f, 0);
    device->BeginScene();

    // Camera — static for debugging
    D3DMATRIX view, proj;
    float eyeX = 4.f, eyeY = 3.f, eyeZ = -4.f;
    MakeLookAtLH(eyeX, eyeY, eyeZ, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, view);
    MakePerspectiveFovLH(3.14159265f / 4.f, (float)app.width / (float)app.height, 0.1f, 50.f, proj);
    device->SetTransform(D3DTS_VIEW, &view);
    device->SetTransform(D3DTS_PROJECTION, &proj);

    D3DVIEWPORT9 vp = { 0, 0, app.width, app.height, 0.f, 1.f };
    device->SetViewport(&vp);

    // --- Draw cube ---
    D3DMATRIX world;
    MakeIdentity(world);
    device->SetTransform(D3DTS_WORLD, &world);

    // Rotate the light around the scene
    float lAngle = time * 0.5f;
    D3DLIGHT9 light{};
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse  = { 1.0f, 0.95f, 0.8f, 1.0f };
    light.Specular = { 1.0f, 1.0f,  1.0f, 1.0f };
    light.Direction.x = cosf(lAngle) * 0.7f;
    light.Direction.y = -1.0f;
    light.Direction.z = sinf(lAngle) * 0.7f;
    device->SetLight(0, &light);
    device->LightEnable(0, TRUE);

    D3DMATERIAL9 cubeMat{};
    cubeMat.Diffuse  = { 1.0f, 1.0f, 1.0f, 1.0f };
    cubeMat.Specular = { 0.6f, 0.6f, 0.6f, 1.0f };
    cubeMat.Power    = 20.f;
    cubeMat.Ambient  = { 1.0f, 1.0f, 1.0f, 1.0f };
    device->SetMaterial(&cubeMat);

    device->SetTexture(0, (IDirect3DTexture9*)app.texCube);
    device->SetFVF(FVF_CUBE);
    device->SetStreamSource(0, (IDirect3DVertexBuffer9*)app.vbCube, 0, sizeof(CubeVertex));
    device->SetIndices((IDirect3DIndexBuffer9*)app.ibCube);
    device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);

    // --- Draw floor ---
    MakeIdentity(world);
    device->SetTransform(D3DTS_WORLD, &world);

    D3DMATERIAL9 floorMat{};
    floorMat.Diffuse  = { 1.0f, 1.0f, 1.0f, 1.0f };
    floorMat.Specular = { 0.0f, 0.0f, 0.0f, 1.0f };
    floorMat.Power    = 1.f;
    floorMat.Ambient  = { 0.5f, 0.5f, 0.5f, 1.0f };
    device->SetMaterial(&floorMat);

    device->SetTexture(0, (IDirect3DTexture9*)app.texFloor);
    device->SetFVF(FVF_FLOOR);
    device->SetStreamSource(0, (IDirect3DVertexBuffer9*)app.vbFloor, 0, sizeof(FloorVertex));
    device->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);

    // --- Draw light indicator (unlit yellow octahedron) ---
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetTexture(0, nullptr);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);

    // Place at 3× normalized light direction (pointing FROM light toward scene)
    float lx = light.Direction.x, ly = light.Direction.y, lz = light.Direction.z;
    float ll = sqrtf(lx*lx + ly*ly + lz*lz);
    float lightPosX = -(lx/ll) * 3.f;
    float lightPosY = -(ly/ll) * 3.f;
    float lightPosZ = -(lz/ll) * 3.f;

    float sz = 0.15f;
    struct ColorVert { float x, y, z; DWORD color; };
    DWORD yellow = 0xFFFFFF00;
    ColorVert octaVerts[] = {
        {lightPosX + sz, lightPosY,      lightPosZ,      yellow},
        {lightPosX - sz, lightPosY,      lightPosZ,      yellow},
        {lightPosX,      lightPosY + sz, lightPosZ,      yellow},
        {lightPosX,      lightPosY - sz, lightPosZ,      yellow},
        {lightPosX,      lightPosY,      lightPosZ + sz, yellow},
        {lightPosX,      lightPosY,      lightPosZ - sz, yellow},
    };
    uint16_t octaIdx[] = {
        0,2,4, 0,4,3, 0,3,5, 0,5,2,
        1,4,2, 1,3,4, 1,5,3, 1,2,5,
    };

    device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 6, 8,
        octaIdx, D3DFMT_INDEX16, octaVerts, sizeof(ColorVert));

    // Restore state for next frame
    device->SetRenderState(D3DRS_LIGHTING, TRUE);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

    device->EndScene();
    device->Present(nullptr, nullptr, nullptr, nullptr);
}

void D3D9SceneShutdown(D3D9SceneApp& app)
{
    auto release = [](void*& p) { if (p) { ((IUnknown*)p)->Release(); p = nullptr; } };
    release(app.texFloor);
    release(app.texCube);
    release(app.vbFloor);
    release(app.ibCube);
    release(app.vbCube);
    release(app.device);
    release(app.d3d9);
}
