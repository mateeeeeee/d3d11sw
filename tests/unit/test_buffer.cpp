#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "resources/buffer.h"

using namespace d3d11sw;

struct BufferTests : ::testing::Test
{
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* context = nullptr;

    void SetUp() override
    {
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
        ASSERT_TRUE(SUCCEEDED(hr));
        ASSERT_NE(device, nullptr);
        ASSERT_NE(context, nullptr);
    }

    void TearDown() override
    {
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }
};

TEST_F(BufferTests, CreationNoInitialData)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = 64;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    ID3D11Buffer* buffer = nullptr;
    HRESULT hr = device->CreateBuffer(&desc, nullptr, &buffer);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(buffer, nullptr);

    D3D11_BUFFER_DESC outDesc = {};
    buffer->GetDesc(&outDesc);
    EXPECT_EQ(outDesc.ByteWidth, 64u);
    EXPECT_EQ(outDesc.Usage,     D3D11_USAGE_DEFAULT);
    EXPECT_EQ(outDesc.BindFlags, D3D11_BIND_VERTEX_BUFFER);

    buffer->Release();
}

TEST_F(BufferTests, CreationWithInitialData)
{
    UINT8 data[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = sizeof(data);
    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;

    ID3D11Buffer* buffer = nullptr;
    HRESULT hr = device->CreateBuffer(&desc, &initData, &buffer);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(buffer, nullptr);

    buffer->Release();
}

TEST_F(BufferTests, MapUnmapRoundtrip)
{
    UINT8 data[32] = {};
    for (int i = 0; i < 32; i++) data[i] = (UINT8)i;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = sizeof(data);
    desc.Usage          = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;

    ID3D11Buffer* buffer = nullptr;
    HRESULT hr = device->CreateBuffer(&desc, &initData, &buffer);
    ASSERT_TRUE(SUCCEEDED(hr));

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    hr = context->Map(buffer, 0, D3D11_MAP_WRITE, 0, &mapped);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(mapped.pData, nullptr);

    UINT8* ptr = static_cast<UINT8*>(mapped.pData);
    for (int i = 0; i < 32; i++) ptr[i] = (UINT8)(i * 2);
    context->Unmap(buffer, 0);

    hr = context->Map(buffer, 0, D3D11_MAP_READ, 0, &mapped);
    ASSERT_TRUE(SUCCEEDED(hr));
    ptr = static_cast<UINT8*>(mapped.pData);
    for (int i = 0; i < 32; i++) EXPECT_EQ(ptr[i], (UINT8)(i * 2));
    context->Unmap(buffer, 0);

    buffer->Release();
}

TEST_F(BufferTests, NullDescRejected)
{
    HRESULT hr = device->CreateBuffer(nullptr, nullptr, nullptr);
    EXPECT_TRUE(FAILED(hr));
}

TEST_F(BufferTests, NullOutputReturnsSFalse)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = 64;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    EXPECT_EQ(device->CreateBuffer(&desc, nullptr, nullptr), S_FALSE);
}

TEST_F(BufferTests, SubresourceCount)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = 64;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    ID3D11Buffer* buffer = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buffer)));

    EXPECT_EQ(static_cast<D3D11BufferSW*>(buffer)->GetSubresourceCount(), 1u);

    buffer->Release();
}

TEST_F(BufferTests, DataSize)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = 128;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    ID3D11Buffer* buffer = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buffer)));

    EXPECT_EQ(static_cast<D3D11BufferSW*>(buffer)->GetDataSize(), 128ull);

    buffer->Release();
}

TEST_F(BufferTests, SubresourceLayout)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = 256;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    ID3D11Buffer* buffer = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buffer)));

    auto layout = static_cast<D3D11BufferSW*>(buffer)->GetSubresourceLayout(0);
    EXPECT_EQ(layout.Offset,     0ull);
    EXPECT_EQ(layout.RowPitch,   256u);
    EXPECT_EQ(layout.DepthPitch, 256u);
    EXPECT_EQ(layout.NumRows,    1u);
    EXPECT_EQ(layout.NumSlices,  1u);

    buffer->Release();
}

TEST_F(BufferTests, DepthStencilBindRejected)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = 64;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Buffer* buffer = nullptr;
    EXPECT_TRUE(FAILED(device->CreateBuffer(&desc, nullptr, &buffer)));
}

TEST_F(BufferTests, StructuredBufferZeroStrideRejected)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth           = 64;
    desc.Usage               = D3D11_USAGE_DEFAULT;
    desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = 0; // invalid

    ID3D11Buffer* buffer = nullptr;
    EXPECT_TRUE(FAILED(device->CreateBuffer(&desc, nullptr, &buffer)));
}
