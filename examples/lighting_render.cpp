#define COM_NO_WINDOWS_H
#include <windows.h>
#include <d3d11_4.h>
#include <dxgi1_2.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include "lighting_render.h"

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
struct Vec4 { float x, y, z, w; };

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

static void MakeRotationY(float angle, float* m)
{
    MakeIdentity(m);
    float c = cosf(angle), s = sinf(angle);
    m[0] = c;  m[2] = s;
    m[8] = -s; m[10] = c;
}

static void MakeTranslation(float tx, float ty, float tz, float* m)
{
    MakeIdentity(m);
    m[12] = tx; m[13] = ty; m[14] = tz;
}

static void MakeScale(float sx, float sy, float sz, float* m)
{
    MakeIdentity(m);
    m[0] = sx; m[5] = sy; m[10] = sz;
}

static void MakePerspectiveFovLH(float fovY, float aspect, float zn, float zf, float* m)
{
    memset(m, 0, 16 * sizeof(float));
    float h = 1.f / tanf(fovY * 0.5f);
    m[0] = h / aspect;
    m[5] = h;
    m[10] = zf / (zf - zn);
    m[11] = 1.f;
    m[14] = -(zn * zf) / (zf - zn);
}

static void MakeLookAtLH(Vec3 eye, Vec3 target, Vec3 up, float* m)
{
    Vec3 f = { target.x - eye.x, target.y - eye.y, target.z - eye.z };
    float fl = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
    f.x /= fl; f.y /= fl; f.z /= fl;

    Vec3 s = { up.y * f.z - up.z * f.y, up.z * f.x - up.x * f.z, up.x * f.y - up.y * f.x };
    float sl = sqrtf(s.x * s.x + s.y * s.y + s.z * s.z);
    s.x /= sl; s.y /= sl; s.z /= sl;

    Vec3 u = { f.y * s.z - f.z * s.y, f.z * s.x - f.x * s.z, f.x * s.y - f.y * s.x };

    MakeIdentity(m);
    m[0] = s.x;  m[1] = u.x;  m[2]  = f.x;
    m[4] = s.y;  m[5] = u.y;  m[6]  = f.y;
    m[8] = s.z;  m[9] = u.z;  m[10] = f.z;
    m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
    m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    m[14] = -(f.x * eye.x + f.y * eye.y + f.z * eye.z);
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

static Vec3 Vec3Transform(Vec3 v, const float* m)
{
    float x = v.x * m[0] + v.y * m[4] + v.z * m[8]  + m[12];
    float y = v.x * m[1] + v.y * m[5] + v.z * m[9]  + m[13];
    float z = v.x * m[2] + v.y * m[6] + v.z * m[10] + m[14];
    return {x, y, z};
}

struct alignas(16) ConstantBuffer
{
    float mWorld[16];
    float mView[16];
    float mProjection[16];
    Vec4  vLightDir[2];
    Vec4  vLightColor[2];
    Vec4  vOutputColor;
};

bool LightingInit(LightingApp& app, void* nsWindow, uint32_t width, uint32_t height)
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
    dsTexDesc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsTexDesc.SampleDesc.Count = 1;
    dsTexDesc.Usage            = D3D11_USAGE_DEFAULT;
    dsTexDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* dsTex = nullptr;
    device->CreateTexture2D(&dsTexDesc, nullptr, &dsTex);
    app.dsTex = dsTex;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    ID3D11DepthStencilView* dsv = nullptr;
    device->CreateDepthStencilView(dsTex, &dsvDesc, &dsv);
    app.dsv = dsv;

    struct Vertex { float x, y, z, nx, ny, nz; };
    Vertex vertices[] = {
        {-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f},
        { 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f},
        { 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f},
        {-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f},

        {-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f},
        { 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f},
        { 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f},
        {-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f},

        {-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f},
        {-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f},
        {-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f},
        {-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f},

        { 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f},
        { 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f},
        { 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f},
        { 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f},

        {-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f},

        {-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f},
        { 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f},
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
         3,  1,  0,   2,  1,  3,
         6,  4,  5,   7,  4,  6,
        11,  9,  8,  10,  9, 11,
        14, 12, 13,  15, 12, 14,
        19, 17, 16,  18, 17, 19,
        22, 20, 21,  23, 20, 22,
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
    cbDesc.ByteWidth = sizeof(ConstantBuffer);
    cbDesc.Usage     = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    ID3D11Buffer* cb = nullptr;
    device->CreateBuffer(&cbDesc, nullptr, &cb);
    app.cb = cb;

    auto vsBlob = ReadFile(D3D11SW_SHADER_DIR "/vs_lighting.o");
    auto psLitBlob = ReadFile(D3D11SW_SHADER_DIR "/ps_lighting.o");
    auto psSolidBlob = ReadFile(D3D11SW_SHADER_DIR "/ps_solid_color.o");

    ID3D11VertexShader* vs = nullptr;
    device->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, &vs);
    app.vs = vs;

    ID3D11PixelShader* psLit = nullptr;
    device->CreatePixelShader(psLitBlob.data(), psLitBlob.size(), nullptr, &psLit);
    app.psLighting = psLit;

    ID3D11PixelShader* psSolid = nullptr;
    device->CreatePixelShader(psSolidBlob.data(), psSolidBlob.size(), nullptr, &psSolid);
    app.psSolid = psSolid;

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* layout = nullptr;
    device->CreateInputLayout(layoutDesc, 2, vsBlob.data(), vsBlob.size(), &layout);
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
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;
    ID3D11RasterizerState* rsState = nullptr;
    device->CreateRasterizerState(&rsDesc, &rsState);
    app.rsState = rsState;

    return true;
}

void LightingRenderFrame(LightingApp& app, float time)
{
    auto* ctx     = (ID3D11DeviceContext*)app.context;
    auto* sc      = (IDXGISwapChain*)app.swapChain;
    auto* rtv     = (ID3D11RenderTargetView*)app.rtv;
    auto* dsv     = (ID3D11DepthStencilView*)app.dsv;
    auto* vs      = (ID3D11VertexShader*)app.vs;
    auto* psLit   = (ID3D11PixelShader*)app.psLighting;
    auto* psSolid = (ID3D11PixelShader*)app.psSolid;
    auto* layout  = (ID3D11InputLayout*)app.layout;
    auto* vb      = (ID3D11Buffer*)app.vb;
    auto* ib      = (ID3D11Buffer*)app.ib;
    auto* cb      = (ID3D11Buffer*)app.cb;

    float worldMat[16];
    MakeRotationY(time, worldMat);

    Vec4 vLightDirs[2] = {
        { -0.577f, 0.577f, -0.577f, 1.0f },
        {  0.0f,   0.0f,   -1.0f,   1.0f },
    };
    Vec4 vLightColors[2] = {
        { 0.5f, 0.5f, 0.5f, 1.0f },
        { 0.5f, 0.0f, 0.0f, 1.0f },
    };

    float rotLight[16];
    MakeRotationY(-2.0f * time, rotLight);
    Vec3 rotDir = Vec3Transform({vLightDirs[1].x, vLightDirs[1].y, vLightDirs[1].z}, rotLight);
    vLightDirs[1] = { rotDir.x, rotDir.y, rotDir.z, 1.0f };

    float clearColor[] = { 0.098039f, 0.098039f, 0.439216f, 1.0f };
    ctx->ClearRenderTargetView(rtv, clearColor);
    ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

    float view[16], proj[16];
    MakeLookAtLH({0.0f, 4.0f, -10.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, view);
    MakePerspectiveFovLH(3.14159265f / 4.0f, (float)app.width / (float)app.height, 0.01f, 100.0f, proj);

    ConstantBuffer cb1{};
    Transpose4x4(worldMat, cb1.mWorld);
    Transpose4x4(view, cb1.mView);
    Transpose4x4(proj, cb1.mProjection);
    cb1.vLightDir[0]   = vLightDirs[0];
    cb1.vLightDir[1]   = vLightDirs[1];
    cb1.vLightColor[0] = vLightColors[0];
    cb1.vLightColor[1] = vLightColors[1];
    cb1.vOutputColor   = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->UpdateSubresource(cb, 0, nullptr, &cb1, 0, 0);

    ctx->OMSetRenderTargets(1, &rtv, dsv);
    ctx->OMSetDepthStencilState((ID3D11DepthStencilState*)app.dsState, 0);
    ctx->RSSetState((ID3D11RasterizerState*)app.rsState);

    D3D11_VIEWPORT viewport{};
    viewport.Width    = (float)app.width;
    viewport.Height   = (float)app.height;
    viewport.MaxDepth = 1.f;
    ctx->RSSetViewports(1, &viewport);

    UINT stride = 24;
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    ctx->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetInputLayout(layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ctx->VSSetShader(vs, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &cb);
    ctx->PSSetShader(psLit, nullptr, 0);
    ctx->PSSetConstantBuffers(0, 1, &cb);
    ctx->DrawIndexed(36, 0, 0);

    for (int m = 0; m < 2; m++)
    {
        float lightScale[16], lightTranslate[16], lightWorld[16];
        MakeScale(0.2f, 0.2f, 0.2f, lightScale);
        MakeTranslation(
            5.0f * vLightDirs[m].x,
            5.0f * vLightDirs[m].y,
            5.0f * vLightDirs[m].z,
            lightTranslate);
        MatMul4x4(lightScale, lightTranslate, lightWorld);

        Transpose4x4(lightWorld, cb1.mWorld);
        cb1.vOutputColor = vLightColors[m];
        ctx->UpdateSubresource(cb, 0, nullptr, &cb1, 0, 0);

        ctx->PSSetShader(psSolid, nullptr, 0);
        ctx->DrawIndexed(36, 0, 0);
    }

    sc->Present(0, 0);
}

void LightingShutdown(LightingApp& app)
{
    auto release = [](void*& p) { if (p) { ((IUnknown*)p)->Release(); p = nullptr; } };
    release(app.rsState);
    release(app.dsState);
    release(app.cb);
    release(app.ib);
    release(app.vb);
    release(app.layout);
    release(app.psSolid);
    release(app.psLighting);
    release(app.vs);
    release(app.dsv);
    release(app.dsTex);
    release(app.rtv);
    release(app.backbuffer);
    release(app.swapChain);
    release(app.context);
    release(app.device);
}
