#define COM_NO_WINDOWS_H
#include <windows.h>
#include <d3d11_4.h>
#include <dxgi1_2.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include "floor_render.h"

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

static void MakeIdentity(float* m)
{
    memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

static void MakeLookAt(float ex, float ey, float ez,
                        float tx, float ty, float tz,
                        float ux, float uy, float uz, float* m)
{
    float fx = tx - ex, fy = ty - ey, fz = tz - ez;
    float fl = sqrtf(fx*fx + fy*fy + fz*fz);
    fx /= fl; fy /= fl; fz /= fl;
    float sx = fy*uz - fz*uy, sy = fz*ux - fx*uz, sz = fx*uy - fy*ux;
    float sl = sqrtf(sx*sx + sy*sy + sz*sz);
    sx /= sl; sy /= sl; sz /= sl;
    float upx = sy*fz - sz*fy, upy = sz*fx - sx*fz, upz = sx*fy - sy*fx;
    MakeIdentity(m);
    m[0]=sx;  m[1]=upx; m[2]=-fx;
    m[4]=sy;  m[5]=upy; m[6]=-fy;
    m[8]=sz;  m[9]=upz; m[10]=-fz;
    m[12]=-(sx*ex+sy*ey+sz*ez);
    m[13]=-(upx*ex+upy*ey+upz*ez);
    m[14]=(fx*ex+fy*ey+fz*ez);
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

bool FloorInit(FloorApp& app, void* nsWindow, uint32_t width, uint32_t height)
{
    app.width = width;
    app.height = height;
    app.autoLod = true;

    DXGI_SWAP_CHAIN_DESC1 scDesc{};
    scDesc.Width       = width;
    scDesc.Height      = height;
    scDesc.Format      = DXGI_FORMAT_B8G8R8A8_UNORM;
    scDesc.SampleDesc  = {1, 0};
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 1;
    scDesc.SwapEffect  = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    D3D_FEATURE_LEVEL fl;
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                      nullptr, 0, D3D11_SDK_VERSION, &dev, &fl, &ctx);

    IDXGIDevice* dxgiDev = nullptr;
    dev->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDev);
    IDXGIAdapter* adapter = nullptr;
    dxgiDev->GetAdapter(&adapter);
    IDXGIFactory2* factory = nullptr;
    adapter->GetParent(__uuidof(IDXGIFactory2), (void**)&factory);

    IDXGISwapChain1* sc = nullptr;
    factory->CreateSwapChainForHwnd((IUnknown*)dev, (HWND)nsWindow, &scDesc, nullptr, nullptr, &sc);

    factory->Release();
    adapter->Release();
    dxgiDev->Release();

    ID3D11Texture2D* bb = nullptr;
    sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);

    app.device    = dev;
    app.context   = ctx;
    app.swapChain = sc;
    app.backbuffer = bb;

    D3D11_TEXTURE2D_DESC dsDesc{};
    dsDesc.Width            = width;
    dsDesc.Height           = height;
    dsDesc.MipLevels        = 1;
    dsDesc.ArraySize        = 1;
    dsDesc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsDesc.SampleDesc.Count = 1;
    dsDesc.Usage            = D3D11_USAGE_DEFAULT;
    dsDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* dsTex = nullptr;
    dev->CreateTexture2D(&dsDesc, nullptr, &dsTex);
    app.depthTex = dsTex;

    ID3D11DepthStencilView* dsv = nullptr;
    dev->CreateDepthStencilView(dsTex, nullptr, &dsv);
    app.depthDSV = dsv;

    auto vsData = ReadFile(D3D11SW_SHADER_DIR "/vs_floor.o");
    auto psAutoData = ReadFile(D3D11SW_SHADER_DIR "/ps_floor_autolod.o");
    auto psLod0Data = ReadFile(D3D11SW_SHADER_DIR "/ps_floor_lod0.o");

    ID3D11VertexShader* vs = nullptr;
    dev->CreateVertexShader(vsData.data(), vsData.size(), nullptr, &vs);
    app.vs = vs;

    ID3D11PixelShader* psAuto = nullptr;
    dev->CreatePixelShader(psAutoData.data(), psAutoData.size(), nullptr, &psAuto);
    app.psAutoLod = psAuto;

    ID3D11PixelShader* psLod0 = nullptr;
    dev->CreatePixelShader(psLod0Data.data(), psLod0Data.size(), nullptr, &psLod0);
    app.psLod0 = psLod0;

    D3D11_INPUT_ELEMENT_DESC elems[] = {
        {"POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    ID3D11InputLayout* layout = nullptr;
    dev->CreateInputLayout(elems, 2, vsData.data(), vsData.size(), &layout);
    app.layout = layout;

    float verts[] = {
        -10.f, 0.f,  1.f,   0.f,  0.f,
         10.f, 0.f, 50.f,  20.f, 50.f,
         10.f, 0.f,  1.f,  20.f,  0.f,
        -10.f, 0.f,  1.f,   0.f,  0.f,
        -10.f, 0.f, 50.f,   0.f, 50.f,
         10.f, 0.f, 50.f,  20.f, 50.f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(verts);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = verts;
    ID3D11Buffer* vb = nullptr;
    dev->CreateBuffer(&vbDesc, &vbInit, &vb);
    app.vb = vb;

    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = 64;
    cbDesc.Usage     = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    ID3D11Buffer* cb = nullptr;
    dev->CreateBuffer(&cbDesc, nullptr, &cb);
    app.cb = cb;

    const uint32_t texW = 64, texH = 64;
    const uint32_t mipCount = 7;
    uint8_t mip0[texW * texH * 4];
    for (uint32_t y = 0; y < texH; ++y)
    {
        for (uint32_t x = 0; x < texW; ++x)
        {
            int i = (y * texW + x) * 4;
            bool white = ((x / 4) + (y / 4)) % 2 == 0;
            mip0[i + 0] = white ? 255 : 40;
            mip0[i + 1] = white ? 255 : 40;
            mip0[i + 2] = white ? 255 : 40;
            mip0[i + 3] = 255;
        }
    }

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width            = texW;
    texDesc.Height           = texH;
    texDesc.MipLevels        = 0;
    texDesc.ArraySize        = 1;
    texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage            = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    texDesc.MiscFlags        = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    ID3D11Texture2D* tex = nullptr;
    dev->CreateTexture2D(&texDesc, nullptr, &tex);
    ctx->UpdateSubresource(tex, 0, nullptr, mip0, texW * 4, 0);
    app.tex = tex;

    ID3D11ShaderResourceView* srv = nullptr;
    dev->CreateShaderResourceView(tex, nullptr, &srv);
    app.srv = srv;

    ctx->GenerateMips(srv);

    D3D11_SAMPLER_DESC smpDesc{};
    smpDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    smpDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    smpDesc.MaxLOD   = D3D11_FLOAT32_MAX;
    ID3D11SamplerState* sampler = nullptr;
    dev->CreateSamplerState(&smpDesc, &sampler);
    app.sampler = sampler;

    D3D11_DEPTH_STENCIL_DESC dssDesc{};
    dssDesc.DepthEnable    = TRUE;
    dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dssDesc.DepthFunc      = D3D11_COMPARISON_LESS;
    ID3D11DepthStencilState* dss = nullptr;
    dev->CreateDepthStencilState(&dssDesc, &dss);
    app.dsState = dss;

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = TRUE;
    ID3D11RasterizerState* rs = nullptr;
    dev->CreateRasterizerState(&rsDesc, &rs);
    app.rsState = rs;

    return true;
}

void FloorRenderFrame(FloorApp& app, float time)
{
    auto* dev = (ID3D11Device*)app.device;
    auto* ctx = (ID3D11DeviceContext*)app.context;
    auto* sc  = (IDXGISwapChain1*)app.swapChain;
    auto* bb  = (ID3D11Texture2D*)app.backbuffer;

    ID3D11RenderTargetView* rtv = nullptr;
    dev->CreateRenderTargetView(bb, nullptr, &rtv);

    float clearColor[] = {0.3f, 0.5f, 0.8f, 1.f};
    ctx->ClearRenderTargetView(rtv, clearColor);
    ctx->ClearDepthStencilView((ID3D11DepthStencilView*)app.depthDSV,
                                D3D11_CLEAR_DEPTH, 1.f, 0);

    float view[16], proj[16], vp[16], vpT[16];
    MakeLookAt(0.f, 3.f, 0.f, 0.f, 0.f, 10.f, 0.f, 1.f, 0.f, view);
    MakePerspective(1.0f, (float)app.width / (float)app.height, 0.1f, 100.f, proj);
    MatMul4x4(view, proj, vp);
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            vpT[c * 4 + r] = vp[r * 4 + c];
        }
    }
    ctx->UpdateSubresource((ID3D11Buffer*)app.cb, 0, nullptr, vpT, 0, 0);

    D3D11_VIEWPORT viewport{};
    viewport.Width    = (float)app.width;
    viewport.Height   = (float)app.height;
    viewport.MaxDepth = 1.f;

    auto* dsv = (ID3D11DepthStencilView*)app.depthDSV;
    ctx->OMSetRenderTargets(1, &rtv, dsv);
    ctx->OMSetDepthStencilState((ID3D11DepthStencilState*)app.dsState, 0);
    ctx->RSSetState((ID3D11RasterizerState*)app.rsState);
    ctx->RSSetViewports(1, &viewport);

    ctx->VSSetShader((ID3D11VertexShader*)app.vs, nullptr, 0);
    ID3D11PixelShader* ps = app.autoLod
        ? (ID3D11PixelShader*)app.psAutoLod
        : (ID3D11PixelShader*)app.psLod0;
    ctx->PSSetShader(ps, nullptr, 0);

    auto* cbPtr = (ID3D11Buffer*)app.cb;
    ctx->VSSetConstantBuffers(0, 1, &cbPtr);
    auto* srvPtr = (ID3D11ShaderResourceView*)app.srv;
    ctx->PSSetShaderResources(0, 1, &srvPtr);
    auto* smpPtr = (ID3D11SamplerState*)app.sampler;
    ctx->PSSetSamplers(0, 1, &smpPtr);

    ctx->IASetInputLayout((ID3D11InputLayout*)app.layout);
    auto* vbPtr = (ID3D11Buffer*)app.vb;
    UINT stride = 20, offset = 0;
    ctx->IASetVertexBuffers(0, 1, &vbPtr, &stride, &offset);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ctx->Draw(6, 0);

    rtv->Release();
    sc->Present(0, 0);
}

void FloorToggleLod(FloorApp& app)
{
    app.autoLod = !app.autoLod;
    fprintf(stderr, "LOD mode: %s\n", app.autoLod ? "Auto (Sample)" : "Fixed 0 (SampleLevel)");
}

void FloorShutdown(FloorApp& app)
{
    auto rel = [](void*& p) { if (p) { ((IUnknown*)p)->Release(); p = nullptr; } };
    rel(app.rsState);
    rel(app.dsState);
    rel(app.sampler);
    rel(app.srv);
    rel(app.tex);
    rel(app.cb);
    rel(app.vb);
    rel(app.layout);
    rel(app.psLod0);
    rel(app.psAutoLod);
    rel(app.vs);
    rel(app.depthDSV);
    rel(app.depthTex);
    rel(app.backbuffer);
    rel(app.swapChain);
    rel(app.context);
    rel(app.device);
}
