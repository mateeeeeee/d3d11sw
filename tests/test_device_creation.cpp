#include <d3d11_4.h>
#include <cassert>
#include <cstdio>

//Break into google tests

int main() 
{
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &device,
        &featureLevel,
        &context);

    assert(SUCCEEDED(hr));
    assert(device != nullptr);
    assert(context != nullptr);

    printf("d3d11sw: Device created successfully!\n");
    printf("d3d11sw: Feature level: 0x%x\n", featureLevel);

    ID3D11Device1* device1 = nullptr;
    hr = device->QueryInterface(__uuidof(ID3D11Device1), (void**)&device1);
    if (SUCCEEDED(hr)) 
    {
        printf("d3d11sw: QueryInterface for ID3D11Device1 succeeded\n");
        device1->Release();
    }
    
    context->Release();
    device->Release();

        // --- Buffer creation ---
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth      = 64;
        desc.Usage          = D3D11_USAGE_DEFAULT;
        desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;

        ID3D11Buffer* buffer = nullptr;
        hr = device->CreateBuffer(&desc, nullptr, &buffer);
        assert(SUCCEEDED(hr));
        assert(buffer != nullptr);

        D3D11_BUFFER_DESC outDesc = {};
        buffer->GetDesc(&outDesc);
        assert(outDesc.ByteWidth == 64);
        assert(outDesc.Usage     == D3D11_USAGE_DEFAULT);
        assert(outDesc.BindFlags == D3D11_BIND_VERTEX_BUFFER);

        buffer->Release();
        printf("d3d11sw: Buffer creation (no initial data) passed!\n");
    }

    // --- Buffer creation with initial data ---
    {
        UINT8 data[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(data);
        desc.Usage     = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = data;

        ID3D11Buffer* buffer = nullptr;
        hr = device->CreateBuffer(&desc, &initData, &buffer);
        assert(SUCCEEDED(hr));
        assert(buffer != nullptr);

        buffer->Release();
        printf("d3d11sw: Buffer creation (with initial data) passed!\n");
    }

    // --- Map / Unmap roundtrip ---
    {
        UINT8 data[32] = {};
        for (int i = 0; i < 32; i++) data[i] = (UINT8)i;

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth      = sizeof(data);
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = data;

        ID3D11Buffer* buffer = nullptr;
        hr = device->CreateBuffer(&desc, &initData, &buffer);
        assert(SUCCEEDED(hr));

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        hr = context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        assert(SUCCEEDED(hr));
        assert(mapped.pData != nullptr);

        UINT8* ptr = static_cast<UINT8*>(mapped.pData);
        for (int i = 0; i < 32; i++) ptr[i] = (UINT8)(i * 2);
        context->Unmap(buffer, 0);

        // map again read back
        hr = context->Map(buffer, 0, D3D11_MAP_READ, 0, &mapped);
        assert(SUCCEEDED(hr));
        ptr = static_cast<UINT8*>(mapped.pData);
        for (int i = 0; i < 32; i++) assert(ptr[i] == (UINT8)(i * 2));
        context->Unmap(buffer, 0);

        buffer->Release();
        printf("d3d11sw: Buffer Map/Unmap roundtrip passed!\n");
    }

    // --- Invalid args ---
    {
        hr = device->CreateBuffer(nullptr, nullptr, nullptr);
        assert(FAILED(hr));
        printf("d3d11sw: Buffer null desc rejected correctly!\n");
    }

    printf("d3d11sw: All tests passed!\n");
    return 0;
}
