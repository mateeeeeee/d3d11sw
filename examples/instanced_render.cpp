#define COM_NO_WINDOWS_H
#include <windows.h>
#include <d3d11_4.h>
#include <dxgi1_2.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include "instanced_render.h"

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
    {
        for (int c = 0; c < 4; ++c)
        {
            float sum = 0.f;
            for (int k = 0; k < 4; ++k)
            {
                sum += a[r * 4 + k] * b[k * 4 + c];
            }
            out[r * 4 + c] = sum;
        }
    }
}

static void MakeIdentity(float* m)
{
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

static void MakeTranslation(float tx, float ty, float tz, float* m)
{
    MakeIdentity(m);
    m[12] = tx; m[13] = ty; m[14] = tz;
}

static void MakeRotationY(float angle, float* m)
{
    MakeIdentity(m);
    float c = cosf(angle), s = sinf(angle);
    m[0] = c;  m[2] = s;
    m[8] = -s; m[10] = c;
}

static void MakeRotationX(float angle, float* m)
{
    MakeIdentity(m);
    float c = cosf(angle), s = sinf(angle);
    m[5] = c;  m[6] = -s;
    m[9] = s;  m[10] = c;
}

static void MakeScale(float sx, float sy, float sz, float* m)
{
    MakeIdentity(m);
    m[0] = sx; m[5] = sy; m[10] = sz;
}

static void MakePerspective(float fovY, float aspect, float zn, float zf, float* m)
{
    memset(m, 0, 16 * sizeof(float));
    float h = 1.f / tanf(fovY * 0.5f);
    m[0] = h / aspect;
    m[5] = h;
    m[10] = zf / (zn - zf);
    m[11] = -1.f;
    m[14] = (zn * zf) / (zn - zf);
}

static void MakeLookAt(Vec3 eye, Vec3 target, Vec3 up, float* m)
{
    Vec3 f = { target.x - eye.x, target.y - eye.y, target.z - eye.z };
    float fl = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
    f.x /= fl; f.y /= fl; f.z /= fl;

    Vec3 s = { f.y * up.z - f.z * up.y, f.z * up.x - f.x * up.z, f.x * up.y - f.y * up.x };
    float sl = sqrtf(s.x * s.x + s.y * s.y + s.z * s.z);
    s.x /= sl; s.y /= sl; s.z /= sl;

    Vec3 u = { s.y * f.z - s.z * f.y, s.z * f.x - s.x * f.z, s.x * f.y - s.y * f.x };

    MakeIdentity(m);
    m[0] = s.x;  m[1] = u.x;  m[2]  = -f.x;
    m[4] = s.y;  m[5] = u.y;  m[6]  = -f.y;
    m[8] = s.z;  m[9] = u.z;  m[10] = -f.z;
    m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
    m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    m[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
}

static void Transpose4x4(const float* in, float* out)
{
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            out[c * 4 + r] = in[r * 4 + c];
        }
    }
}

static constexpr int GRID = 3;
static constexpr int NUM_INSTANCES = GRID * GRID * GRID;

struct CBuffer
{
    float viewProj[16];
    float world[NUM_INSTANCES][16];
};

bool InstancedInit(InstancedApp& app, void* nsWindow, uint32_t width, uint32_t height)
{
    app.width  = width;
    app.height = height;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    IDXGISwapChain* swapChain = nullptr;

    DXGI_SWAP_CHAIN_DESC scDesc{};
    scDesc.BufferDesc.Width  = width;
    scDesc.BufferDesc.Height = height;
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.SampleDesc        = { 1, 0 };
    scDesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount       = 1;
    scDesc.OutputWindow      = (HWND)nsWindow;
    scDesc.Windowed          = TRUE;
    scDesc.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &scDesc, &swapChain, &device, nullptr, &ctx);

    if (FAILED(hr))
    {
        fprintf(stderr, "D3D11CreateDeviceAndSwapChain failed: 0x%08X\n", (unsigned)hr);
        return false;
    }

    app.device    = device;
    app.context   = ctx;
    app.swapChain = swapChain;

    ID3D11Texture2D* backbuffer = nullptr;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer);
    app.backbuffer = backbuffer;

    ID3D11RenderTargetView* rtv = nullptr;
    device->CreateRenderTargetView(backbuffer, nullptr, &rtv);
    app.rtv = rtv;

    D3D11_TEXTURE2D_DESC dsTexDesc{};
    dsTexDesc.Width            = width;
    dsTexDesc.Height           = height;
    dsTexDesc.MipLevels        = 1;
    dsTexDesc.ArraySize        = 1;
    dsTexDesc.Format           = DXGI_FORMAT_D32_FLOAT;
    dsTexDesc.SampleDesc.Count = 1;
    dsTexDesc.Usage            = D3D11_USAGE_DEFAULT;
    dsTexDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* dsTex = nullptr;
    device->CreateTexture2D(&dsTexDesc, nullptr, &dsTex);
    app.dsTex = dsTex;

    ID3D11DepthStencilView* dsv = nullptr;
    device->CreateDepthStencilView(dsTex, nullptr, &dsv);
    app.dsv = dsv;

    const uint32_t TW = 4, TH = 4;
    uint8_t texData[TW * TH * 4];
    for (uint32_t y = 0; y < TH; ++y)
    {
        for (uint32_t x = 0; x < TW; ++x)
        {
            uint8_t* p = texData + (y * TW + x) * 4;
            uint8_t v = ((x + y) & 1) ? 80 : 255;
            p[0] = v; p[1] = v; p[2] = v; p[3] = 255;
        }
    }

    D3D11_TEXTURE2D_DESC srcTexDesc{};
    srcTexDesc.Width            = TW;
    srcTexDesc.Height           = TH;
    srcTexDesc.MipLevels        = 1;
    srcTexDesc.ArraySize        = 1;
    srcTexDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    srcTexDesc.SampleDesc.Count = 1;
    srcTexDesc.Usage            = D3D11_USAGE_DEFAULT;
    srcTexDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA texInit{};
    texInit.pSysMem     = texData;
    texInit.SysMemPitch = TW * 4;

    ID3D11Texture2D* srcTex = nullptr;
    device->CreateTexture2D(&srcTexDesc, &texInit, &srcTex);

    ID3D11ShaderResourceView* srv = nullptr;
    device->CreateShaderResourceView(srcTex, nullptr, &srv);
    srcTex->Release();
    app.srv = srv;

    D3D11_SAMPLER_DESC smpDesc{};
    smpDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
    smpDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.MaxLOD   = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* sampler = nullptr;
    device->CreateSamplerState(&smpDesc, &sampler);
    app.sampler = sampler;

    struct Vertex { float x, y, z, nx, ny, nz, u, v; };
    Vertex vertices[] = {
        {-0.5f, -0.5f,  0.5f,  0, 0, 1,  0.f, 1.f},
        { 0.5f, -0.5f,  0.5f,  0, 0, 1,  1.f, 1.f},
        { 0.5f,  0.5f,  0.5f,  0, 0, 1,  1.f, 0.f},
        {-0.5f,  0.5f,  0.5f,  0, 0, 1,  0.f, 0.f},
        { 0.5f, -0.5f, -0.5f,  0, 0,-1,  0.f, 1.f},
        {-0.5f, -0.5f, -0.5f,  0, 0,-1,  1.f, 1.f},
        {-0.5f,  0.5f, -0.5f,  0, 0,-1,  1.f, 0.f},
        { 0.5f,  0.5f, -0.5f,  0, 0,-1,  0.f, 0.f},
        { 0.5f, -0.5f,  0.5f,  1, 0, 0,  0.f, 1.f},
        { 0.5f, -0.5f, -0.5f,  1, 0, 0,  1.f, 1.f},
        { 0.5f,  0.5f, -0.5f,  1, 0, 0,  1.f, 0.f},
        { 0.5f,  0.5f,  0.5f,  1, 0, 0,  0.f, 0.f},
        {-0.5f, -0.5f, -0.5f, -1, 0, 0,  0.f, 1.f},
        {-0.5f, -0.5f,  0.5f, -1, 0, 0,  1.f, 1.f},
        {-0.5f,  0.5f,  0.5f, -1, 0, 0,  1.f, 0.f},
        {-0.5f,  0.5f, -0.5f, -1, 0, 0,  0.f, 0.f},
        {-0.5f,  0.5f,  0.5f,  0, 1, 0,  0.f, 1.f},
        { 0.5f,  0.5f,  0.5f,  0, 1, 0,  1.f, 1.f},
        { 0.5f,  0.5f, -0.5f,  0, 1, 0,  1.f, 0.f},
        {-0.5f,  0.5f, -0.5f,  0, 1, 0,  0.f, 0.f},
        {-0.5f, -0.5f, -0.5f,  0,-1, 0,  0.f, 1.f},
        { 0.5f, -0.5f, -0.5f,  0,-1, 0,  1.f, 1.f},
        { 0.5f, -0.5f,  0.5f,  0,-1, 0,  1.f, 0.f},
        {-0.5f, -0.5f,  0.5f,  0,-1, 0,  0.f, 0.f},
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;
    ID3D11Buffer* vb = nullptr;
    device->CreateBuffer(&vbDesc, &vbInit, &vb);
    app.vb = vb;

    uint16_t indices[] = {
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23,
    };

    D3D11_BUFFER_DESC ibDesc{};
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.Usage     = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibInit{};
    ibInit.pSysMem = indices;
    ID3D11Buffer* ib = nullptr;
    device->CreateBuffer(&ibDesc, &ibInit, &ib);
    app.ib = ib;

    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = sizeof(CBuffer);
    cbDesc.Usage     = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    ID3D11Buffer* cb = nullptr;
    device->CreateBuffer(&cbDesc, nullptr, &cb);
    app.cb = cb;

    auto vsBlob = ReadFile(D3D11SW_SHADER_DIR "/vs_instanced_world.o");
    auto psBlob = ReadFile(D3D11SW_SHADER_DIR "/ps_textured_normal.o");

    ID3D11VertexShader* vs = nullptr;
    device->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, &vs);
    app.vs = vs;

    ID3D11PixelShader* ps = nullptr;
    device->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, &ps);
    app.ps = ps;

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* layout = nullptr;
    device->CreateInputLayout(layoutDesc, 3, vsBlob.data(), vsBlob.size(), &layout);
    app.layout = layout;

    D3D11_DEPTH_STENCIL_DESC dsStateDesc{};
    dsStateDesc.DepthEnable    = TRUE;
    dsStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsStateDesc.DepthFunc      = D3D11_COMPARISON_LESS;
    ID3D11DepthStencilState* dsState = nullptr;
    device->CreateDepthStencilState(&dsStateDesc, &dsState);
    app.dsState = dsState;

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode        = D3D11_FILL_SOLID;
    rsDesc.CullMode        = D3D11_CULL_BACK;
    rsDesc.DepthClipEnable = TRUE;
    ID3D11RasterizerState* rsState = nullptr;
    device->CreateRasterizerState(&rsDesc, &rsState);
    app.rsState = rsState;

    return true;
}

void InstancedRenderFrame(InstancedApp& app, float time)
{
    auto* ctx     = (ID3D11DeviceContext*)app.context;
    auto* sc      = (IDXGISwapChain*)app.swapChain;
    auto* rtv     = (ID3D11RenderTargetView*)app.rtv;
    auto* dsv     = (ID3D11DepthStencilView*)app.dsv;
    auto* vs      = (ID3D11VertexShader*)app.vs;
    auto* ps      = (ID3D11PixelShader*)app.ps;
    auto* layout  = (ID3D11InputLayout*)app.layout;
    auto* vb      = (ID3D11Buffer*)app.vb;
    auto* ib      = (ID3D11Buffer*)app.ib;
    auto* cb      = (ID3D11Buffer*)app.cb;
    auto* srv     = (ID3D11ShaderResourceView*)app.srv;
    auto* sampler = (ID3D11SamplerState*)app.sampler;

    CBuffer cbData;

    float view[16], proj[16], vp[16];
    float camDist = 6.f;
    float camAngle = time * 0.2f;
    Vec3 eye = { sinf(camAngle) * camDist, 3.f, cosf(camAngle) * camDist };
    MakeLookAt(eye, {0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, view);
    MakePerspective(3.14159f / 3.f, (float)app.width / (float)app.height, 0.1f, 50.f, proj);
    MatMul4x4(view, proj, vp);
    Transpose4x4(vp, cbData.viewProj);

    int idx = 0;
    float spacing = 2.0f;
    float offset = -(GRID - 1) * spacing * 0.5f;
    for (int z = 0; z < GRID; ++z)
    {
        for (int y = 0; y < GRID; ++y)
        {
            for (int x = 0; x < GRID; ++x)
            {
                float tx = offset + x * spacing;
                float ty = offset + y * spacing;
                float tz = offset + z * spacing;

                float seed = (float)(x * 7 + y * 13 + z * 19);
                float speed = 0.5f + seed * 0.02f;

                float rotYm[16], rotXm[16], rot[16], trans[16], scale[16];
                float sr[16], world[16];

                MakeRotationY(time * speed + seed, rotYm);
                MakeRotationX(time * speed * 0.7f + seed * 1.3f, rotXm);
                MatMul4x4(rotXm, rotYm, rot);

                MakeScale(0.6f, 0.6f, 0.6f, scale);
                MatMul4x4(scale, rot, sr);

                MakeTranslation(tx, ty, tz, trans);
                MatMul4x4(sr, trans, world);

                Transpose4x4(world, cbData.world[idx]);
                ++idx;
            }
        }
    }

    ctx->UpdateSubresource(cb, 0, nullptr, &cbData, 0, 0);

    float clearColor[] = { 0.1f, 0.1f, 0.15f, 1.0f };
    ctx->ClearRenderTargetView(rtv, clearColor);
    ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

    ctx->OMSetRenderTargets(1, &rtv, dsv);
    ctx->OMSetDepthStencilState((ID3D11DepthStencilState*)app.dsState, 0);
    ctx->RSSetState((ID3D11RasterizerState*)app.rsState);

    D3D11_VIEWPORT viewport{};
    viewport.Width    = (float)app.width;
    viewport.Height   = (float)app.height;
    viewport.MaxDepth = 1.f;
    ctx->RSSetViewports(1, &viewport);

    UINT stride = 32;
    UINT offset2 = 0;
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset2);
    ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetInputLayout(layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(vs, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &cb);
    ctx->PSSetShader(ps, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &srv);
    ctx->PSSetSamplers(0, 1, &sampler);

    ctx->DrawIndexedInstanced(36, NUM_INSTANCES, 0, 0, 0);

    sc->Present(0, 0);
}

void InstancedShutdown(InstancedApp& app)
{
    auto release = [](void*& p) { if (p) { ((IUnknown*)p)->Release(); p = nullptr; } };
    release(app.rsState);
    release(app.dsState);
    release(app.sampler);
    release(app.srv);
    release(app.cb);
    release(app.ib);
    release(app.vb);
    release(app.layout);
    release(app.ps);
    release(app.vs);
    release(app.dsv);
    release(app.dsTex);
    release(app.rtv);
    release(app.backbuffer);
    release(app.swapChain);
    release(app.context);
    release(app.device);
}
