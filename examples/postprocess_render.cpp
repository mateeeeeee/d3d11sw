#define COM_NO_WINDOWS_H
#include <windows.h>
#include <d3d11_4.h>
#include <dxgi1_2.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include "postprocess_render.h"

static std::vector<uint8_t> ReadFile(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        fprintf(stderr, "Failed to open: %s\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> data(size);
    fread(data.data(), 1, size, f);
    fclose(f);
    return data;
}

struct Vec3 { float x, y, z; };

static void MatMul4x4(const float* a, const float* b, float* out)
{
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
        {
            float sum = 0.f;
            for (int k = 0; k < 4; ++k)
                sum += a[r * 4 + k] * b[k * 4 + c];
            out[r * 4 + c] = sum;
        }
}

static void MakeIdentity(float* m)
{
    memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

static void MakeRotationY(float a, float* m)
{
    MakeIdentity(m);
    float c = cosf(a), s = sinf(a);
    m[0] = c; m[2] = s; m[8] = -s; m[10] = c;
}

static void MakeRotationX(float a, float* m)
{
    MakeIdentity(m);
    float c = cosf(a), s = sinf(a);
    m[5] = c; m[6] = -s; m[9] = s; m[10] = c;
}

static void MakePerspective(float fovY, float aspect, float zn, float zf, float* m)
{
    memset(m, 0, 64);
    float h = 1.f / tanf(fovY * 0.5f);
    m[0] = h / aspect; m[5] = h;
    m[10] = zf / (zn - zf); m[11] = -1.f;
    m[14] = (zn * zf) / (zn - zf);
}

static void MakeLookAt(Vec3 eye, Vec3 target, Vec3 up, float* m)
{
    Vec3 f = { target.x - eye.x, target.y - eye.y, target.z - eye.z };
    float fl = sqrtf(f.x*f.x + f.y*f.y + f.z*f.z);
    f.x /= fl; f.y /= fl; f.z /= fl;
    Vec3 s = { f.y*up.z - f.z*up.y, f.z*up.x - f.x*up.z, f.x*up.y - f.y*up.x };
    float sl = sqrtf(s.x*s.x + s.y*s.y + s.z*s.z);
    s.x /= sl; s.y /= sl; s.z /= sl;
    Vec3 u = { s.y*f.z - s.z*f.y, s.z*f.x - s.x*f.z, s.x*f.y - s.y*f.x };
    MakeIdentity(m);
    m[0] = s.x; m[1] = u.x; m[2] = -f.x;
    m[4] = s.y; m[5] = u.y; m[6] = -f.y;
    m[8] = s.z; m[9] = u.z; m[10] = -f.z;
    m[12] = -(s.x*eye.x + s.y*eye.y + s.z*eye.z);
    m[13] = -(u.x*eye.x + u.y*eye.y + u.z*eye.z);
    m[14] = (f.x*eye.x + f.y*eye.y + f.z*eye.z);
}

static void Transpose4x4(const float* in, float* out)
{
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out[c * 4 + r] = in[r * 4 + c];
}

static const uint32_t PROC_TEX_SIZE = 64;

bool PostProcessInit(PostProcessApp& app, void* nsWindow, uint32_t width, uint32_t height)
{
    app.width = width;
    app.height = height;

    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    IDXGISwapChain* sc = nullptr;

    DXGI_SWAP_CHAIN_DESC scDesc{};
    scDesc.BufferDesc.Width = width;
    scDesc.BufferDesc.Height = height;
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.SampleDesc = { 1, 0 };
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 1;
    scDesc.OutputWindow = (HWND)nsWindow;
    scDesc.Windowed = TRUE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION, &scDesc, &sc, &dev, nullptr, &ctx)))
    {
        return false;
    }

    app.device = dev;
    app.context = ctx;
    app.swapChain = sc;

    ID3D11Texture2D* bb = nullptr;
    sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    app.backbuffer = bb;

    // Offscreen scene RT (render target + shader resource)
    D3D11_TEXTURE2D_DESC rtDesc{};
    rtDesc.Width = width;
    rtDesc.Height = height;
    rtDesc.MipLevels = 1;
    rtDesc.ArraySize = 1;
    rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtDesc.SampleDesc.Count = 1;
    rtDesc.Usage = D3D11_USAGE_DEFAULT;
    rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* sceneRT = nullptr;
    dev->CreateTexture2D(&rtDesc, nullptr, &sceneRT);
    app.sceneRT = sceneRT;

    ID3D11RenderTargetView* sceneRTV = nullptr;
    dev->CreateRenderTargetView(sceneRT, nullptr, &sceneRTV);
    app.sceneRTV = sceneRTV;

    ID3D11ShaderResourceView* sceneSRV = nullptr;
    dev->CreateShaderResourceView(sceneRT, nullptr, &sceneSRV);
    app.sceneSRV = sceneSRV;

    // Post-process output (UAV, then copied to backbuffer)
    D3D11_TEXTURE2D_DESC postDesc = rtDesc;
    postDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    ID3D11Texture2D* postTex = nullptr;
    dev->CreateTexture2D(&postDesc, nullptr, &postTex);
    app.postTex = postTex;

    ID3D11UnorderedAccessView* postUAV = nullptr;
    dev->CreateUnorderedAccessView(postTex, nullptr, &postUAV);
    app.postUAV = postUAV;

    // Procedural texture (UAV for compute write, SRV for draw read)
    D3D11_TEXTURE2D_DESC procDesc{};
    procDesc.Width = PROC_TEX_SIZE;
    procDesc.Height = PROC_TEX_SIZE;
    procDesc.MipLevels = 1;
    procDesc.ArraySize = 1;
    procDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    procDesc.SampleDesc.Count = 1;
    procDesc.Usage = D3D11_USAGE_DEFAULT;
    procDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* procTex = nullptr;
    dev->CreateTexture2D(&procDesc, nullptr, &procTex);
    app.procTex = procTex;

    ID3D11UnorderedAccessView* procUAV = nullptr;
    dev->CreateUnorderedAccessView(procTex, nullptr, &procUAV);
    app.procUAV = procUAV;

    ID3D11ShaderResourceView* procSRV = nullptr;
    dev->CreateShaderResourceView(procTex, nullptr, &procSRV);
    app.procSRV = procSRV;

    // Depth buffer
    D3D11_TEXTURE2D_DESC dsDesc{};
    dsDesc.Width = width;
    dsDesc.Height = height;
    dsDesc.MipLevels = 1;
    dsDesc.ArraySize = 1;
    dsDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsDesc.SampleDesc.Count = 1;
    dsDesc.Usage = D3D11_USAGE_DEFAULT;
    dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthTex = nullptr;
    dev->CreateTexture2D(&dsDesc, nullptr, &depthTex);
    app.depthTex = depthTex;

    ID3D11DepthStencilView* depthDSV = nullptr;
    dev->CreateDepthStencilView(depthTex, nullptr, &depthDSV);
    app.depthDSV = depthDSV;

    // Sampler
    D3D11_SAMPLER_DESC smpDesc{};
    smpDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    smpDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.MaxLOD = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* sampler = nullptr;
    dev->CreateSamplerState(&smpDesc, &sampler);
    app.sampler = sampler;

    // Cube geometry
    struct Vertex { float x, y, z, nx, ny, nz, u, v; };
    Vertex vertices[] = {
        {-0.5f,-0.5f, 0.5f,  0, 0, 1, 0,1}, { 0.5f,-0.5f, 0.5f,  0, 0, 1, 1,1},
        { 0.5f, 0.5f, 0.5f,  0, 0, 1, 1,0}, {-0.5f, 0.5f, 0.5f,  0, 0, 1, 0,0},
        { 0.5f,-0.5f,-0.5f,  0, 0,-1, 0,1}, {-0.5f,-0.5f,-0.5f,  0, 0,-1, 1,1},
        {-0.5f, 0.5f,-0.5f,  0, 0,-1, 1,0}, { 0.5f, 0.5f,-0.5f,  0, 0,-1, 0,0},
        { 0.5f,-0.5f, 0.5f,  1, 0, 0, 0,1}, { 0.5f,-0.5f,-0.5f,  1, 0, 0, 1,1},
        { 0.5f, 0.5f,-0.5f,  1, 0, 0, 1,0}, { 0.5f, 0.5f, 0.5f,  1, 0, 0, 0,0},
        {-0.5f,-0.5f,-0.5f, -1, 0, 0, 0,1}, {-0.5f,-0.5f, 0.5f, -1, 0, 0, 1,1},
        {-0.5f, 0.5f, 0.5f, -1, 0, 0, 1,0}, {-0.5f, 0.5f,-0.5f, -1, 0, 0, 0,0},
        {-0.5f, 0.5f, 0.5f,  0, 1, 0, 0,1}, { 0.5f, 0.5f, 0.5f,  0, 1, 0, 1,1},
        { 0.5f, 0.5f,-0.5f,  0, 1, 0, 1,0}, {-0.5f, 0.5f,-0.5f,  0, 1, 0, 0,0},
        {-0.5f,-0.5f,-0.5f,  0,-1, 0, 0,1}, { 0.5f,-0.5f,-0.5f,  0,-1, 0, 1,1},
        { 0.5f,-0.5f, 0.5f,  0,-1, 0, 1,0}, {-0.5f,-0.5f, 0.5f,  0,-1, 0, 0,0},
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{}; vbInit.pSysMem = vertices;
    ID3D11Buffer* vb = nullptr;
    dev->CreateBuffer(&vbDesc, &vbInit, &vb);
    app.vb = vb;

    uint16_t indices[] = {
        0,2,1, 0,3,2, 4,6,5, 4,7,6, 8,10,9, 8,11,10,
        12,14,13, 12,15,14, 16,18,17, 16,19,18, 20,22,21, 20,23,22,
    };
    D3D11_BUFFER_DESC ibDesc{};
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibInit{}; ibInit.pSysMem = indices;
    ID3D11Buffer* ib = nullptr;
    dev->CreateBuffer(&ibDesc, &ibInit, &ib);
    app.ib = ib;

    // Constant buffers
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    cbDesc.ByteWidth = 64;
    ID3D11Buffer* cbScene = nullptr;
    dev->CreateBuffer(&cbDesc, nullptr, &cbScene);
    app.cbScene = cbScene;

    cbDesc.ByteWidth = 16;
    ID3D11Buffer* cbProc = nullptr;
    dev->CreateBuffer(&cbDesc, nullptr, &cbProc);
    app.cbProc = cbProc;

    ID3D11Buffer* cbPost = nullptr;
    dev->CreateBuffer(&cbDesc, nullptr, &cbPost);
    app.cbPost = cbPost;

    // Shaders
    auto vsBlob = ReadFile(D3D11SW_SHADER_DIR "/vs_transform_texcoord.o");
    auto psBlob = ReadFile(D3D11SW_SHADER_DIR "/ps_textured_normal.o");
    auto csProcBlob = ReadFile(D3D11SW_SHADER_DIR "/cs_procedural.o");
    auto csPostBlob = ReadFile(D3D11SW_SHADER_DIR "/cs_postprocess.o");

    ID3D11VertexShader* vs = nullptr;
    dev->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, &vs);
    app.vs = vs;

    ID3D11PixelShader* ps = nullptr;
    dev->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, &ps);
    app.ps = ps;

    ID3D11ComputeShader* csProc = nullptr;
    dev->CreateComputeShader(csProcBlob.data(), csProcBlob.size(), nullptr, &csProc);
    app.csProc = csProc;

    ID3D11ComputeShader* csPost = nullptr;
    dev->CreateComputeShader(csPostBlob.data(), csPostBlob.size(), nullptr, &csPost);
    app.csPost = csPost;

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* layout = nullptr;
    dev->CreateInputLayout(layoutDesc, 3, vsBlob.data(), vsBlob.size(), &layout);
    app.layout = layout;

    D3D11_DEPTH_STENCIL_DESC dssDesc{};
    dssDesc.DepthEnable = TRUE;
    dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dssDesc.DepthFunc = D3D11_COMPARISON_LESS;
    ID3D11DepthStencilState* dsState = nullptr;
    dev->CreateDepthStencilState(&dssDesc, &dsState);
    app.dsState = dsState;

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK;
    rsDesc.DepthClipEnable = TRUE;
    ID3D11RasterizerState* rsState = nullptr;
    dev->CreateRasterizerState(&rsDesc, &rsState);
    app.rsState = rsState;

    return true;
}

void PostProcessRenderFrame(PostProcessApp& app, float time)
{
    auto* dev = (ID3D11Device*)app.device;
    auto* ctx = (ID3D11DeviceContext*)app.context;
    auto* sc = (IDXGISwapChain*)app.swapChain;

    // --- Pass 1: Compute generates procedural texture ---
    {
        float cbData[] = { time, (float)PROC_TEX_SIZE, (float)PROC_TEX_SIZE, 0.f };
        ctx->UpdateSubresource((ID3D11Buffer*)app.cbProc, 0, nullptr, cbData, 0, 0);

        auto* uav = (ID3D11UnorderedAccessView*)app.procUAV;
        auto* cb = (ID3D11Buffer*)app.cbProc;
        ctx->CSSetShader((ID3D11ComputeShader*)app.csProc, nullptr, 0);
        ctx->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
        ctx->CSSetConstantBuffers(0, 1, &cb);
        ctx->Dispatch(PROC_TEX_SIZE / 8, PROC_TEX_SIZE / 8, 1);

        ID3D11UnorderedAccessView* nullUAV = nullptr;
        ctx->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
    }

    // --- Pass 2: Draw cube with procedural texture to offscreen RT ---
    {
        auto* rtv = (ID3D11RenderTargetView*)app.sceneRTV;
        auto* dsv = (ID3D11DepthStencilView*)app.depthDSV;

        float clearColor[] = { 0.15f, 0.15f, 0.2f, 1.0f };
        ctx->ClearRenderTargetView(rtv, clearColor);
        ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

        float rotY[16], rotX[16], rot[16], view[16], proj[16], vp[16], mvp[16], mvpT[16];
        MakeRotationY(time * 0.7f, rotY);
        MakeRotationX(time * 0.3f, rotX);
        MatMul4x4(rotX, rotY, rot);
        MakeLookAt({0,0,2.5f}, {0,0,0}, {0,1,0}, view);
        MakePerspective(3.14159f/3.f, (float)app.width/(float)app.height, 0.1f, 10.f, proj);
        MatMul4x4(view, proj, vp);
        MatMul4x4(rot, vp, mvp);
        Transpose4x4(mvp, mvpT);
        ctx->UpdateSubresource((ID3D11Buffer*)app.cbScene, 0, nullptr, mvpT, 0, 0);

        ctx->OMSetRenderTargets(1, &rtv, dsv);
        ctx->OMSetDepthStencilState((ID3D11DepthStencilState*)app.dsState, 0);
        ctx->RSSetState((ID3D11RasterizerState*)app.rsState);

        D3D11_VIEWPORT viewport{};
        viewport.Width = (float)app.width;
        viewport.Height = (float)app.height;
        viewport.MaxDepth = 1.f;
        ctx->RSSetViewports(1, &viewport);

        UINT stride = 32, offset = 0;
        auto* vb = (ID3D11Buffer*)app.vb;
        auto* ib = (ID3D11Buffer*)app.ib;
        auto* cb = (ID3D11Buffer*)app.cbScene;
        auto* srv = (ID3D11ShaderResourceView*)app.procSRV;
        auto* smp = (ID3D11SamplerState*)app.sampler;
        ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);
        ctx->IASetInputLayout((ID3D11InputLayout*)app.layout);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader((ID3D11VertexShader*)app.vs, nullptr, 0);
        ctx->VSSetConstantBuffers(0, 1, &cb);
        ctx->PSSetShader((ID3D11PixelShader*)app.ps, nullptr, 0);
        ctx->PSSetShaderResources(0, 1, &srv);
        ctx->PSSetSamplers(0, 1, &smp);

        ctx->DrawIndexed(36, 0, 0);

        ID3D11RenderTargetView* nullRTV = nullptr;
        ctx->OMSetRenderTargets(1, &nullRTV, nullptr);
    }

    // --- Pass 3: Compute post-processes scene to postTex ---
    {
        float cbData[] = { (float)app.width, (float)app.height, time, 0.f };
        ctx->UpdateSubresource((ID3D11Buffer*)app.cbPost, 0, nullptr, cbData, 0, 0);

        auto* srv = (ID3D11ShaderResourceView*)app.sceneSRV;
        auto* uav = (ID3D11UnorderedAccessView*)app.postUAV;
        auto* cb = (ID3D11Buffer*)app.cbPost;
        ctx->CSSetShader((ID3D11ComputeShader*)app.csPost, nullptr, 0);
        ctx->CSSetShaderResources(0, 1, &srv);
        ctx->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
        ctx->CSSetConstantBuffers(0, 1, &cb);
        ctx->Dispatch((app.width + 7) / 8, (app.height + 7) / 8, 1);

        ID3D11UnorderedAccessView* nullUAV = nullptr;
        ID3D11ShaderResourceView* nullSRV = nullptr;
        ctx->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
        ctx->CSSetShaderResources(0, 1, &nullSRV);
    }

    // --- Copy post-processed result to backbuffer and present ---
    ctx->CopyResource((ID3D11Texture2D*)app.backbuffer, (ID3D11Texture2D*)app.postTex);
    sc->Present(0, 0);
}

void PostProcessShutdown(PostProcessApp& app)
{
    auto release = [](void*& p) { if (p) { ((IUnknown*)p)->Release(); p = nullptr; } };
    release(app.rsState);
    release(app.dsState);
    release(app.sampler);
    release(app.cbPost);
    release(app.cbProc);
    release(app.cbScene);
    release(app.ib);
    release(app.vb);
    release(app.layout);
    release(app.csPost);
    release(app.csProc);
    release(app.ps);
    release(app.vs);
    release(app.procSRV);
    release(app.procUAV);
    release(app.procTex);
    release(app.postUAV);
    release(app.postTex);
    release(app.sceneSRV);
    release(app.sceneRTV);
    release(app.sceneRT);
    release(app.depthDSV);
    release(app.depthTex);
    release(app.backbuffer);
    release(app.swapChain);
    release(app.context);
    release(app.device);
}
