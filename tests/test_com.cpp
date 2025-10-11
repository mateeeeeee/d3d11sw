#include <gtest/gtest.h>
#include <d3d11_4.h>

#include "resources/resource_sw.h"

struct ComTests : ::testing::Test
{
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* context = nullptr;
    ID3D11Buffer*        buffer  = nullptr;

    void SetUp() override
    {
        D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &device, &featureLevel, &context);
        ASSERT_TRUE(SUCCEEDED(hr) && device && context);

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = 64;
        desc.Usage     = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        hr = device->CreateBuffer(&desc, nullptr, &buffer);
        ASSERT_TRUE(SUCCEEDED(hr) && buffer);
    }

    void TearDown() override
    {
        if (buffer)  { buffer->Release();  buffer  = nullptr; }
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }
};

TEST_F(ComTests, DeviceIUnknownIdentity)
{
    IUnknown* a = nullptr;
    IUnknown* b = nullptr;

    HRESULT hr = device->QueryInterface(__uuidof(IUnknown), (void**)&a);
    ASSERT_TRUE(SUCCEEDED(hr));

    ID3D11Device5* d5 = nullptr;
    hr = device->QueryInterface(__uuidof(ID3D11Device5), (void**)&d5);
    ASSERT_TRUE(SUCCEEDED(hr));
    hr = d5->QueryInterface(__uuidof(IUnknown), (void**)&b);
    ASSERT_TRUE(SUCCEEDED(hr));

    EXPECT_EQ(a, b);

    a->Release(); b->Release(); d5->Release();
}

TEST_F(ComTests, DeviceENoInterface)
{
    static const GUID bogus = {0x11111111,0,0,{0,0,0,0,0,0,0,1}};
    void* p = (void*)0xdeadbeef;
    HRESULT hr = device->QueryInterface(bogus, &p);
    EXPECT_EQ(hr, E_NOINTERFACE);
    EXPECT_EQ(p, nullptr);
}

TEST_F(ComTests, DeviceEPointer)
{
    HRESULT hr = device->QueryInterface(__uuidof(IUnknown), nullptr);
    EXPECT_EQ(hr, E_POINTER);
}

TEST_F(ComTests, BufferIUnknownIdentityViaResource)
{
    IUnknown* a = nullptr;
    IUnknown* b = nullptr;

    HRESULT hr = buffer->QueryInterface(__uuidof(IUnknown), (void**)&a);
    ASSERT_TRUE(SUCCEEDED(hr));

    ID3D11Resource* res = nullptr;
    hr = buffer->QueryInterface(__uuidof(ID3D11Resource), (void**)&res);
    ASSERT_TRUE(SUCCEEDED(hr));
    hr = res->QueryInterface(__uuidof(IUnknown), (void**)&b);
    ASSERT_TRUE(SUCCEEDED(hr));

    EXPECT_EQ(a, b);

    a->Release(); b->Release(); res->Release();
}

TEST_F(ComTests, BufferIUnknownIdentityViaDeviceChild)
{
    IUnknown* a = nullptr;
    IUnknown* b = nullptr;

    HRESULT hr = buffer->QueryInterface(__uuidof(IUnknown), (void**)&a);
    ASSERT_TRUE(SUCCEEDED(hr));

    ID3D11DeviceChild* child = nullptr;
    hr = buffer->QueryInterface(__uuidof(ID3D11DeviceChild), (void**)&child);
    ASSERT_TRUE(SUCCEEDED(hr));
    hr = child->QueryInterface(__uuidof(IUnknown), (void**)&b);
    ASSERT_TRUE(SUCCEEDED(hr));

    EXPECT_EQ(a, b);

    a->Release(); b->Release(); child->Release();
}

TEST_F(ComTests, BufferIUnknownIdentityViaISWResource)
{
    d3d11sw::IResourceSW* sw = nullptr;
    HRESULT hr = buffer->QueryInterface(__uuidof(d3d11sw::IResourceSW), (void**)&sw);
    ASSERT_TRUE(SUCCEEDED(hr) && sw);
    ASSERT_NE(sw->GetDataPtr(), nullptr);

    IUnknown* a = nullptr;
    IUnknown* b = nullptr;

    hr = buffer->QueryInterface(__uuidof(IUnknown), (void**)&a);
    ASSERT_TRUE(SUCCEEDED(hr));
    hr = sw->QueryInterface(__uuidof(IUnknown), (void**)&b);
    ASSERT_TRUE(SUCCEEDED(hr));

    EXPECT_EQ(a, b);

    a->Release(); b->Release(); sw->Release();
}

TEST_F(ComTests, BufferQIReflexivity)
{
    ID3D11Buffer* buf2 = nullptr;
    HRESULT hr = buffer->QueryInterface(__uuidof(ID3D11Buffer), (void**)&buf2);
    ASSERT_TRUE(SUCCEEDED(hr));
    EXPECT_EQ(buf2, buffer);
    buf2->Release();
}

TEST_F(ComTests, BufferENoInterface)
{
    static const GUID bogus = {0x22222222,0,0,{0,0,0,0,0,0,0,2}};
    void* p = (void*)0xdeadbeef;
    HRESULT hr = buffer->QueryInterface(bogus, &p);
    EXPECT_EQ(hr, E_NOINTERFACE);
    EXPECT_EQ(p, nullptr);
}

TEST_F(ComTests, BufferRefCounting)
{
    ULONG r;
    r = buffer->AddRef();  EXPECT_EQ(r, 2u);
    r = buffer->AddRef();  EXPECT_EQ(r, 3u);
    r = buffer->Release(); EXPECT_EQ(r, 2u);
    r = buffer->Release(); EXPECT_EQ(r, 1u);
}

TEST_F(ComTests, GetDeviceReturnsParent)
{
    ID3D11Device* dev2 = nullptr;
    buffer->GetDevice(&dev2);
    EXPECT_EQ(dev2, device);
    dev2->Release();
}
