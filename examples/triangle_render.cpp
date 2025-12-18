#define COM_NO_WINDOWS_H
#include <windows.h>
#include <d3d11_4.h>
#include <dxgi1_2.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "triangle_render.h"

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

void RunTriangle(void* nsWindow, uint32_t width, uint32_t height)
{
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
        &scDesc, &swapChain,
        &device, nullptr, &ctx);

    if (FAILED(hr))
    {
        fprintf(stderr, "D3D11CreateDeviceAndSwapChain failed: 0x%08X\n", (unsigned)hr);
        return;
    }

    ID3D11Texture2D* backbuffer = nullptr;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer);

    ID3D11RenderTargetView* rtv = nullptr;
    device->CreateRenderTargetView(backbuffer, nullptr, &rtv);

    auto vsBlob = ReadFile(D3D11SW_SHADER_DIR "/vs_passthrough.o");
    auto psBlob = ReadFile(D3D11SW_SHADER_DIR "/ps_solid_red.o");

    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    device->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, &vs);
    device->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, &ps);

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3D11InputLayout* layout = nullptr;
    device->CreateInputLayout(layoutDesc, 1, vsBlob.data(), vsBlob.size(), &layout);

    float vertices[] = {
         0.0f,  0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f,
         0.5f, -0.5f, 0.5f,
    };
    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData{};
    vbData.pSysMem = vertices;
    ID3D11Buffer* vb = nullptr;
    device->CreateBuffer(&vbDesc, &vbData, &vb);

    float clearColor[] = { 0.2f, 0.3f, 0.4f, 1.0f };
    ctx->ClearRenderTargetView(rtv, clearColor);

    ctx->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT vp{};
    vp.Width    = (float)width;
    vp.Height   = (float)height;
    vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);

    UINT stride = 12;
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    ctx->IASetInputLayout(layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(vs, nullptr, 0);
    ctx->PSSetShader(ps, nullptr, 0);

    ctx->Draw(3, 0);

    swapChain->Present(0, 0);

    printf("Triangle rendered and presented!\n");

    vb->Release();
    layout->Release();
    ps->Release();
    vs->Release();
    rtv->Release();
    backbuffer->Release();
    swapChain->Release();
    ctx->Release();
    device->Release();
}
