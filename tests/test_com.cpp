#include <d3d11_4.h>
#include <cassert>
#include <cstdio>

#include "resources/sw_resource.h"

static void pass(const char* name) { printf("  [PASS] %s\n", name); }

int main()
{
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL    featureLevel;

    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &device, &featureLevel, &context);

    assert(SUCCEEDED(hr) && device && context);

    // --- Device: IUnknown identity ---
    // IUnknown pointer from any interface on the same object must be identical
    {
        IUnknown* a = nullptr;
        IUnknown* b = nullptr;

        hr = device->QueryInterface(__uuidof(IUnknown), (void**)&a);
        assert(SUCCEEDED(hr));

        ID3D11Device5* d5 = nullptr;
        hr = device->QueryInterface(__uuidof(ID3D11Device5), (void**)&d5);
        assert(SUCCEEDED(hr));
        hr = d5->QueryInterface(__uuidof(IUnknown), (void**)&b);
        assert(SUCCEEDED(hr));

        assert(a == b);

        a->Release(); b->Release(); d5->Release();
        pass("device IUnknown identity");
    }

    // --- Device: E_NOINTERFACE ---
    {
        static const GUID bogus = {0x11111111,0,0,{0,0,0,0,0,0,0,1}};
        void* p = (void*)0xdeadbeef;
        hr = device->QueryInterface(bogus, &p);
        assert(hr == E_NOINTERFACE);
        assert(p == nullptr);
        pass("device E_NOINTERFACE + ppv zeroed");
    }

    // --- Device: E_POINTER ---
    {
        hr = device->QueryInterface(__uuidof(IUnknown), nullptr);
        assert(hr == E_POINTER);
        pass("device E_POINTER");
    }

    // Create a buffer for all buffer COM tests
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = 64;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    ID3D11Buffer* buffer = nullptr;
    hr = device->CreateBuffer(&desc, nullptr, &buffer);
    assert(SUCCEEDED(hr) && buffer);

    // --- Buffer: IUnknown identity via ID3D11Resource ---
    {
        IUnknown* a = nullptr;
        IUnknown* b = nullptr;

        hr = buffer->QueryInterface(__uuidof(IUnknown), (void**)&a);
        assert(SUCCEEDED(hr));

        ID3D11Resource* res = nullptr;
        hr = buffer->QueryInterface(__uuidof(ID3D11Resource), (void**)&res);
        assert(SUCCEEDED(hr));
        hr = res->QueryInterface(__uuidof(IUnknown), (void**)&b);
        assert(SUCCEEDED(hr));

        assert(a == b);

        a->Release(); b->Release(); res->Release();
        pass("buffer IUnknown identity (via ID3D11Resource)");
    }

    // --- Buffer: IUnknown identity via ID3D11DeviceChild ---
    {
        IUnknown* a = nullptr;
        IUnknown* b = nullptr;

        hr = buffer->QueryInterface(__uuidof(IUnknown), (void**)&a);
        assert(SUCCEEDED(hr));

        ID3D11DeviceChild* child = nullptr;
        hr = buffer->QueryInterface(__uuidof(ID3D11DeviceChild), (void**)&child);
        assert(SUCCEEDED(hr));
        hr = child->QueryInterface(__uuidof(IUnknown), (void**)&b);
        assert(SUCCEEDED(hr));

        assert(a == b);

        a->Release(); b->Release(); child->Release();
        pass("buffer IUnknown identity (via ID3D11DeviceChild)");
    }

    // --- Buffer: IUnknown identity via ISWResource ---
    {
        d3d11sw::ISWResource* sw = nullptr;
        hr = buffer->QueryInterface(__uuidof(d3d11sw::ISWResource), (void**)&sw);
        assert(SUCCEEDED(hr) && sw);
        assert(sw->GetDataPtr() != nullptr);

        IUnknown* a = nullptr;
        IUnknown* b = nullptr;

        hr = buffer->QueryInterface(__uuidof(IUnknown), (void**)&a);
        assert(SUCCEEDED(hr));
        hr = sw->QueryInterface(__uuidof(IUnknown), (void**)&b);
        assert(SUCCEEDED(hr));

        assert(a == b);

        a->Release(); b->Release(); sw->Release();
        pass("buffer IUnknown identity (via ISWResource)");
    }

    // --- Buffer: QI reflexivity — same interface, same pointer ---
    {
        ID3D11Buffer* buf2 = nullptr;
        hr = buffer->QueryInterface(__uuidof(ID3D11Buffer), (void**)&buf2);
        assert(SUCCEEDED(hr));
        assert(buf2 == buffer);
        buf2->Release();
        pass("buffer QI reflexivity");
    }

    // --- Buffer: E_NOINTERFACE + ppv zeroed ---
    {
        static const GUID bogus = {0x22222222,0,0,{0,0,0,0,0,0,0,2}};
        void* p = (void*)0xdeadbeef;
        hr = buffer->QueryInterface(bogus, &p);
        assert(hr == E_NOINTERFACE);
        assert(p == nullptr);
        pass("buffer E_NOINTERFACE + ppv zeroed");
    }

    // --- Buffer: ref counting ---
    {
        ULONG r;
        r = buffer->AddRef();  assert(r == 2);
        r = buffer->AddRef();  assert(r == 3);
        r = buffer->Release(); assert(r == 2);
        r = buffer->Release(); assert(r == 1);
        pass("buffer ref counting");
    }

    // --- GetDevice returns the same device ---
    {
        ID3D11Device* dev2 = nullptr;
        buffer->GetDevice(&dev2);
        assert(dev2 == device);
        dev2->Release();
        pass("GetDevice returns parent device");
    }

    buffer->Release();
    context->Release();
    device->Release();

    printf("\nd3d11sw: All COM tests passed!\n");
    return 0;
}
