#define COM_NO_WINDOWS_H
#include <windows.h>
#include <d3d11_4.h>
#include <dxgi1_2.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include "tessellation_render.h"

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

static void Transpose4x4(const float* src, float* dst)
{
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            dst[c * 4 + r] = src[r * 4 + c];
        }
    }
}

static const int GRID_N = 8;

bool TessInit(TessApp& app, void* nsWindow, uint32_t width, uint32_t height)
{
    app.width  = width;
    app.height = height;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    IDXGISwapChain* swapChain = nullptr;

    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferDesc.Width  = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.SampleDesc.Count  = 1;
    scd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount       = 1;
    scd.OutputWindow      = (HWND)nsWindow;
    scd.Windowed          = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &scd, &swapChain, &device, nullptr, &ctx);
    if (FAILED(hr))
    {
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

    // Grid vertices on XZ plane, centered at origin
    float halfSize = (float)GRID_N * 0.5f;
    std::vector<float> verts;
    for (int z = 0; z <= GRID_N; ++z)
    {
        for (int x = 0; x <= GRID_N; ++x)
        {
            verts.push_back((float)x - halfSize);
            verts.push_back(0.f);
            verts.push_back((float)z - halfSize);
        }
    }

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = (UINT)(verts.size() * sizeof(float));
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = verts.data();
    ID3D11Buffer* vb = nullptr;
    device->CreateBuffer(&vbDesc, &vbInit, &vb);
    app.vb = vb;

    // Patch indices: each quad = 4 control points
    std::vector<uint16_t> indices;
    int stride = GRID_N + 1;
    for (int z = 0; z < GRID_N; ++z)
    {
        for (int x = 0; x < GRID_N; ++x)
        {
            uint16_t tl = (uint16_t)(z * stride + x);
            uint16_t tr = tl + 1;
            uint16_t bl = tl + (uint16_t)stride;
            uint16_t br = bl + 1;
            indices.push_back(tl);
            indices.push_back(tr);
            indices.push_back(br);
            indices.push_back(bl);
        }
    }
    app.indexCount = (uint32_t)indices.size();

    D3D11_BUFFER_DESC ibDesc{};
    ibDesc.ByteWidth = (UINT)(indices.size() * sizeof(uint16_t));
    ibDesc.Usage     = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibInit{};
    ibInit.pSysMem = indices.data();
    ID3D11Buffer* ib = nullptr;
    device->CreateBuffer(&ibDesc, &ibInit, &ib);
    app.ib = ib;

    // Constant buffer: float4 cameraPos + float4x4 viewProj = 80 bytes, padded to 16-byte alignment
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth      = 80;
    cbDesc.Usage          = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    ID3D11Buffer* cb = nullptr;
    device->CreateBuffer(&cbDesc, nullptr, &cb);
    app.cb = cb;

    // Shaders
    auto vsBC = ReadFile(D3D11SW_SHADER_DIR "/vs_tess_grid.o");
    auto hsBC = ReadFile(D3D11SW_SHADER_DIR "/hs_adaptive.o");
    auto dsBC = ReadFile(D3D11SW_SHADER_DIR "/ds_grid_mvp.o");
    auto psBC = ReadFile(D3D11SW_SHADER_DIR "/ps_white.o");

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    ID3D11InputLayout* layout = nullptr;
    device->CreateInputLayout(layoutDesc, 1, vsBC.data(), vsBC.size(), &layout);
    app.layout = layout;

    ID3D11VertexShader* vs = nullptr;
    device->CreateVertexShader(vsBC.data(), vsBC.size(), nullptr, &vs);
    app.vs = vs;

    ID3D11HullShader* hs = nullptr;
    device->CreateHullShader(hsBC.data(), hsBC.size(), nullptr, &hs);
    app.hs = hs;

    ID3D11DomainShader* ds = nullptr;
    device->CreateDomainShader(dsBC.data(), dsBC.size(), nullptr, &ds);
    app.ds = ds;

    ID3D11PixelShader* ps = nullptr;
    device->CreatePixelShader(psBC.data(), psBC.size(), nullptr, &ps);
    app.ps = ps;

    // Wireframe rasterizer state
    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode        = D3D11_FILL_WIREFRAME;
    rsDesc.CullMode        = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = TRUE;
    ID3D11RasterizerState* rs = nullptr;
    device->CreateRasterizerState(&rsDesc, &rs);
    app.rsState = rs;

    // Solid rasterizer state
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK;
    ID3D11RasterizerState* rsSolid = nullptr;
    device->CreateRasterizerState(&rsDesc, &rsSolid);
    app.rsSolid = rsSolid;

    app.camYaw   = 0.f;
    app.camPitch = 0.8f;
    app.camDist  = 8.f;
    app.wireframe = true;

    return true;
}

void TessRenderFrame(TessApp& app, float dt)
{
    auto* ctx = (ID3D11DeviceContext*)app.context;
    auto* rtv = (ID3D11RenderTargetView*)app.rtv;

    float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };
    ctx->ClearRenderTargetView(rtv, clearColor);
    ctx->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT vp{};
    vp.Width    = (float)app.width;
    vp.Height   = (float)app.height;
    vp.MaxDepth = 1.f;
    ctx->RSSetViewports(1, &vp);
    ctx->RSSetState((ID3D11RasterizerState*)(app.wireframe ? app.rsState : app.rsSolid));

    // Camera from yaw/pitch/distance
    float cy = cosf(app.camPitch), sy = sinf(app.camPitch);
    float cx = cosf(app.camYaw),   sx = sinf(app.camYaw);
    Vec3 eye = { app.camDist * cy * sx, app.camDist * sy, app.camDist * cy * cx };
    Vec3 target = { 0.f, 0.f, 0.f };
    Vec3 up = { 0.f, 1.f, 0.f };

    float view[16], proj[16], vp_mat[16];
    MakeLookAt(eye, target, up, view);
    float aspect = (float)app.width / (float)app.height;
    MakePerspective(3.14159f / 4.f, aspect, 0.1f, 50.f, proj);
    MatMul4x4(view, proj, vp_mat);

    // CB layout: float4 cameraPos (16 bytes) + float4x4 viewProj (64 bytes) = 80 bytes
    struct alignas(16) CBData
    {
        float cameraPos[4];
        float viewProj[16];
    } cbData;

    cbData.cameraPos[0] = eye.x;
    cbData.cameraPos[1] = eye.y;
    cbData.cameraPos[2] = eye.z;
    cbData.cameraPos[3] = 0.f;
    Transpose4x4(vp_mat, cbData.viewProj);

    auto* cb = (ID3D11Buffer*)app.cb;
    ctx->UpdateSubresource(cb, 0, nullptr, &cbData, 0, 0);

    // Bind pipeline
    ctx->IASetInputLayout((ID3D11InputLayout*)app.layout);
    UINT stride = 12, offset = 0;
    auto* vb = (ID3D11Buffer*)app.vb;
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    ctx->IASetIndexBuffer((ID3D11Buffer*)app.ib, DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

    ctx->VSSetShader((ID3D11VertexShader*)app.vs, nullptr, 0);
    ctx->HSSetShader((ID3D11HullShader*)app.hs, nullptr, 0);
    ctx->HSSetConstantBuffers(0, 1, &cb);
    ctx->DSSetShader((ID3D11DomainShader*)app.ds, nullptr, 0);
    ctx->DSSetConstantBuffers(0, 1, &cb);
    ctx->PSSetShader((ID3D11PixelShader*)app.ps, nullptr, 0);

    ctx->DrawIndexed(app.indexCount, 0, 0);

    ((IDXGISwapChain*)app.swapChain)->Present(1, 0);
}

void TessShutdown(TessApp& app)
{
    auto rel = [](void*& p) { if (p) { ((IUnknown*)p)->Release(); p = nullptr; } };
    rel(app.rsSolid);
    rel(app.rsState);
    rel(app.ps);
    rel(app.ds);
    rel(app.hs);
    rel(app.vs);
    rel(app.layout);
    rel(app.cb);
    rel(app.ib);
    rel(app.vb);
    rel(app.rtv);
    rel(app.backbuffer);
    rel(app.swapChain);
    rel(app.context);
    rel(app.device);
}
