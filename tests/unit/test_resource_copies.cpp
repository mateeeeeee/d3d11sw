#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <cstring>
#include "resources/buffer.h"
#include "resources/texture2d.h"

using namespace d3d11sw;

struct ResourceCopyTests : ::testing::Test
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

TEST_F(ResourceCopyTests, CopyBuffer_FullCopy)
{
    UINT8 srcData[64];
    for (int i = 0; i < 64; ++i) { srcData[i] = static_cast<UINT8>(i); }

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = 64;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = srcData;

    ID3D11Buffer* src = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, &initData, &src)));

    ID3D11Buffer* dst = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, nullptr, &dst)));

    context->CopyResource(dst, src);

    D3D11_BUFFER_DESC stagingDesc = desc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, dst);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));
    EXPECT_EQ(std::memcmp(mapped.pData, srcData, 64), 0);
    context->Unmap(staging, 0);

    staging->Release();
    dst->Release();
    src->Release();
}

TEST_F(ResourceCopyTests, CopyTexture2D_FullCopy)
{
    const UINT W = 4, H = 4;
    UINT8 srcPixels[W * H * 4];
    for (UINT i = 0; i < W * H * 4; ++i) { srcPixels[i] = static_cast<UINT8>(i & 0xFF); }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width            = W;
    texDesc.Height           = H;
    texDesc.MipLevels        = 1;
    texDesc.ArraySize        = 1;
    texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage            = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags        = D3D11_BIND_RENDER_TARGET;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem     = srcPixels;
    initData.SysMemPitch = W * 4;

    ID3D11Texture2D* src = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, &initData, &src)));

    ID3D11Texture2D* dst = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &dst)));

    context->CopyResource(dst, src);

    D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, dst);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    for (UINT y = 0; y < H; ++y)
    {
        const UINT8* row = static_cast<const UINT8*>(mapped.pData) + y * mapped.RowPitch;
        EXPECT_EQ(std::memcmp(row, srcPixels + y * W * 4, W * 4), 0);
    }
    context->Unmap(staging, 0);

    staging->Release();
    dst->Release();
    src->Release();
}

TEST_F(ResourceCopyTests, UpdateSubresource_Buffer)
{
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = 32;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buf)));

    float data[8] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
    context->UpdateSubresource(buf, 0, nullptr, data, 0, 0);

    D3D11_BUFFER_DESC stagingDesc = desc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, buf);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));
    EXPECT_EQ(std::memcmp(mapped.pData, data, 32), 0);
    context->Unmap(staging, 0);

    staging->Release();
    buf->Release();
}

TEST_F(ResourceCopyTests, UpdateSubresource_Texture2D)
{
    const UINT W = 8, H = 8;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width            = W;
    texDesc.Height           = H;
    texDesc.MipLevels        = 1;
    texDesc.ArraySize        = 1;
    texDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage            = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags        = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* tex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &tex)));

    UINT8 pixels[W * H * 4];
    for (UINT i = 0; i < W * H * 4; ++i) { pixels[i] = static_cast<UINT8>(255 - (i & 0xFF)); }

    context->UpdateSubresource(tex, 0, nullptr, pixels, W * 4, 0);

    D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, tex);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    for (UINT y = 0; y < H; ++y)
    {
        const UINT8* row = static_cast<const UINT8*>(mapped.pData) + y * mapped.RowPitch;
        EXPECT_EQ(std::memcmp(row, pixels + y * W * 4, W * 4), 0);
    }
    context->Unmap(staging, 0);

    staging->Release();
    tex->Release();
}

TEST_F(ResourceCopyTests, CopyBuffer_StagingRoundTrip)
{
    UINT8 data[128];
    for (int i = 0; i < 128; ++i) { data[i] = static_cast<UINT8>(i * 2); }

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = 128;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;

    ID3D11Buffer* gpuBuf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, &initData, &gpuBuf)));

    D3D11_BUFFER_DESC stagingDesc = {};
    stagingDesc.ByteWidth      = 128;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));

    context->CopyResource(staging, gpuBuf);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    const UINT8* result = static_cast<const UINT8*>(mapped.pData);
    for (int i = 0; i < 128; ++i)
    {
        EXPECT_EQ(result[i], static_cast<UINT8>(i * 2)) << "Mismatch at byte " << i;
    }
    context->Unmap(staging, 0);

    staging->Release();
    gpuBuf->Release();
}

TEST_F(ResourceCopyTests, CopyTexture2D_DifferentFormats_SameSize)
{
    const UINT W = 4, H = 4;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width            = W;
    desc.Height           = H;
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_R32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_DEFAULT;
    desc.BindFlags        = D3D11_BIND_RENDER_TARGET;

    float srcData[W * H];
    for (UINT i = 0; i < W * H; ++i) { srcData[i] = static_cast<float>(i) / (W * H); }

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem     = srcData;
    initData.SysMemPitch = W * sizeof(float);

    ID3D11Texture2D* src = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, &initData, &src)));

    ID3D11Texture2D* dst = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &dst)));

    context->CopyResource(dst, src);

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, dst);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    for (UINT y = 0; y < H; ++y)
    {
        const float* row = reinterpret_cast<const float*>(
            static_cast<const UINT8*>(mapped.pData) + y * mapped.RowPitch);
        for (UINT x = 0; x < W; ++x)
        {
            EXPECT_FLOAT_EQ(row[x], srcData[y * W + x]);
        }
    }
    context->Unmap(staging, 0);

    staging->Release();
    dst->Release();
    src->Release();
}
