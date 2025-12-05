#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <cstring>

struct MapModeTests : ::testing::Test
{
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* context = nullptr;

    void SetUp() override
    {
        D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &device, &featureLevel, &context);
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

TEST_F(MapModeTests, Buffer_WriteDiscard)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = 64;
    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buf)));

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = context->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(mapped.pData, nullptr);

    float* data = static_cast<float*>(mapped.pData);
    for (int i = 0; i < 16; ++i) { data[i] = static_cast<float>(i); }
    context->Unmap(buf, 0);

    hr = context->Map(buf, 0, D3D11_MAP_READ, 0, &mapped);
    ASSERT_TRUE(SUCCEEDED(hr));
    const float* readBack = static_cast<const float*>(mapped.pData);
    for (int i = 0; i < 16; ++i)
    {
        EXPECT_FLOAT_EQ(readBack[i], static_cast<float>(i));
    }
    context->Unmap(buf, 0);

    buf->Release();
}

TEST_F(MapModeTests, Buffer_WriteDiscardOverwrites)
{
    UINT8 initData[32];
    for (int i = 0; i < 32; ++i) { initData[i] = 0xAA; }

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = 32;
    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = initData;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, &init, &buf)));

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)));

    UINT8* ptr = static_cast<UINT8*>(mapped.pData);
    for (int i = 0; i < 32; ++i) { ptr[i] = 0xBB; }
    context->Unmap(buf, 0);

    ASSERT_TRUE(SUCCEEDED(context->Map(buf, 0, D3D11_MAP_READ, 0, &mapped)));
    const UINT8* result = static_cast<const UINT8*>(mapped.pData);
    for (int i = 0; i < 32; ++i)
    {
        EXPECT_EQ(result[i], 0xBBu);
    }
    context->Unmap(buf, 0);

    buf->Release();
}

TEST_F(MapModeTests, Staging_Buffer_ReadWrite)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = 64;
    desc.Usage          = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buf)));

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(buf, 0, D3D11_MAP_WRITE, 0, &mapped)));
    UINT32* data = static_cast<UINT32*>(mapped.pData);
    for (int i = 0; i < 16; ++i) { data[i] = static_cast<UINT32>(i * 100); }
    context->Unmap(buf, 0);

    ASSERT_TRUE(SUCCEEDED(context->Map(buf, 0, D3D11_MAP_READ, 0, &mapped)));
    const UINT32* result = static_cast<const UINT32*>(mapped.pData);
    for (int i = 0; i < 16; ++i)
    {
        EXPECT_EQ(result[i], static_cast<UINT32>(i * 100));
    }
    context->Unmap(buf, 0);

    buf->Release();
}

TEST_F(MapModeTests, Staging_Texture2D_ReadWrite)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width            = 8;
    desc.Height           = 8;
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags   = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &tex)));

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(tex, 0, D3D11_MAP_WRITE, 0, &mapped)));
    ASSERT_NE(mapped.pData, nullptr);
    ASSERT_GT(mapped.RowPitch, 0u);

    for (UINT y = 0; y < 8; ++y)
    {
        UINT8* row = static_cast<UINT8*>(mapped.pData) + y * mapped.RowPitch;
        for (UINT x = 0; x < 8; ++x)
        {
            row[x * 4 + 0] = static_cast<UINT8>(x * 32);
            row[x * 4 + 1] = static_cast<UINT8>(y * 32);
            row[x * 4 + 2] = 0;
            row[x * 4 + 3] = 255;
        }
    }
    context->Unmap(tex, 0);

    ASSERT_TRUE(SUCCEEDED(context->Map(tex, 0, D3D11_MAP_READ, 0, &mapped)));
    for (UINT y = 0; y < 8; ++y)
    {
        const UINT8* row = static_cast<const UINT8*>(mapped.pData) + y * mapped.RowPitch;
        for (UINT x = 0; x < 8; ++x)
        {
            EXPECT_EQ(row[x * 4 + 0], static_cast<UINT8>(x * 32));
            EXPECT_EQ(row[x * 4 + 1], static_cast<UINT8>(y * 32));
        }
    }
    context->Unmap(tex, 0);

    tex->Release();
}

TEST_F(MapModeTests, Dynamic_ConstantBuffer_WriteDiscard)
{
    float initData[4] = {1.f, 2.f, 3.f, 4.f};

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = 16;
    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = initData;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, &init, &buf)));

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)));
    float* data = static_cast<float*>(mapped.pData);
    data[0] = 10.f; data[1] = 20.f; data[2] = 30.f; data[3] = 40.f;
    context->Unmap(buf, 0);

    ASSERT_TRUE(SUCCEEDED(context->Map(buf, 0, D3D11_MAP_READ, 0, &mapped)));
    const float* result = static_cast<const float*>(mapped.pData);
    EXPECT_FLOAT_EQ(result[0], 10.f);
    EXPECT_FLOAT_EQ(result[1], 20.f);
    EXPECT_FLOAT_EQ(result[2], 30.f);
    EXPECT_FLOAT_EQ(result[3], 40.f);
    context->Unmap(buf, 0);

    buf->Release();
}

TEST_F(MapModeTests, Buffer_MultipleMapCycles)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth      = 16;
    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buf)));

    for (int cycle = 0; cycle < 5; ++cycle)
    {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        ASSERT_TRUE(SUCCEEDED(context->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)));
        UINT32* data = static_cast<UINT32*>(mapped.pData);
        for (int i = 0; i < 4; ++i) { data[i] = static_cast<UINT32>(cycle * 100 + i); }
        context->Unmap(buf, 0);

        ASSERT_TRUE(SUCCEEDED(context->Map(buf, 0, D3D11_MAP_READ, 0, &mapped)));
        const UINT32* result = static_cast<const UINT32*>(mapped.pData);
        for (int i = 0; i < 4; ++i)
        {
            EXPECT_EQ(result[i], static_cast<UINT32>(cycle * 100 + i));
        }
        context->Unmap(buf, 0);
    }

    buf->Release();
}
