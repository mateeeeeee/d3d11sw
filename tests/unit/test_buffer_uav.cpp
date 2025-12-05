#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <cstring>
#include "resources/buffer.h"
#include "views/unordered_access_view.h"

using namespace d3d11sw;

struct BufferUAVTests : ::testing::Test
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

TEST_F(BufferUAVTests, TypedBuffer_R32Float)
{
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.ByteWidth = 256;
    bufDesc.Usage     = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format              = DXGI_FORMAT_R32_FLOAT;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements  = 64;

    ID3D11UnorderedAccessView* uav = nullptr;
    HRESULT hr = device->CreateUnorderedAccessView(buf, &uavDesc, &uav);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(uav, nullptr);

    D3D11_UNORDERED_ACCESS_VIEW_DESC out = {};
    uav->GetDesc(&out);
    EXPECT_EQ(out.Format, DXGI_FORMAT_R32_FLOAT);
    EXPECT_EQ(out.Buffer.NumElements, 64u);

    uav->Release();
    buf->Release();
}

TEST_F(BufferUAVTests, TypedBuffer_R32G32B32A32Float)
{
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.ByteWidth = 1024;
    bufDesc.Usage     = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format              = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements  = 64;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC out = {};
    uav->GetDesc(&out);
    EXPECT_EQ(out.Format, DXGI_FORMAT_R32G32B32A32_FLOAT);
    EXPECT_EQ(out.Buffer.NumElements, 64u);

    uav->Release();
    buf->Release();
}

TEST_F(BufferUAVTests, RawBuffer)
{
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.ByteWidth = 256;
    bufDesc.Usage     = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format              = DXGI_FORMAT_R32_TYPELESS;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements  = 64;
    uavDesc.Buffer.Flags        = D3D11_BUFFER_UAV_FLAG_RAW;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC out = {};
    uav->GetDesc(&out);
    EXPECT_EQ(out.Format, DXGI_FORMAT_R32_TYPELESS);
    EXPECT_EQ(out.Buffer.Flags, static_cast<UINT>(D3D11_BUFFER_UAV_FLAG_RAW));

    uav->Release();
    buf->Release();
}

TEST_F(BufferUAVTests, StructuredBuffer)
{
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.ByteWidth           = 1024;
    bufDesc.Usage               = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS;
    bufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufDesc.StructureByteStride = 16;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format              = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements  = 64;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC out = {};
    uav->GetDesc(&out);
    EXPECT_EQ(out.Format, DXGI_FORMAT_UNKNOWN);
    EXPECT_EQ(out.Buffer.NumElements, 64u);

    uav->Release();
    buf->Release();
}

TEST_F(BufferUAVTests, StructuredBuffer_SRV)
{
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.ByteWidth           = 512;
    bufDesc.Usage               = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    bufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufDesc.StructureByteStride = 32;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format               = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension        = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement  = 0;
    srvDesc.Buffer.NumElements   = 16;

    ID3D11ShaderResourceView* srv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateShaderResourceView(buf, &srvDesc, &srv)));

    D3D11_SHADER_RESOURCE_VIEW_DESC out = {};
    srv->GetDesc(&out);
    EXPECT_EQ(out.Format, DXGI_FORMAT_UNKNOWN);
    EXPECT_EQ(out.Buffer.NumElements, 16u);

    srv->Release();
    buf->Release();
}

TEST_F(BufferUAVTests, ClearUAV_Buffer_R32Uint)
{
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.ByteWidth = 64 * 4;
    bufDesc.Usage     = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format              = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements  = 64;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

    UINT clearValues[4] = {42, 0, 0, 0};
    context->ClearUnorderedAccessViewUint(uav, clearValues);

    D3D11_BUFFER_DESC stagingDesc = bufDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, buf);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    const UINT32* data = static_cast<const UINT32*>(mapped.pData);
    for (int i = 0; i < 64; ++i)
    {
        EXPECT_EQ(data[i], 42u) << "Mismatch at element " << i;
    }
    context->Unmap(staging, 0);

    staging->Release();
    uav->Release();
    buf->Release();
}

TEST_F(BufferUAVTests, TypedBuffer_R32Uint)
{
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.ByteWidth = 128;
    bufDesc.Usage     = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format              = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements  = 32;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC out = {};
    uav->GetDesc(&out);
    EXPECT_EQ(out.Format, DXGI_FORMAT_R32_UINT);
    EXPECT_EQ(out.Buffer.NumElements, 32u);
    EXPECT_EQ(out.Buffer.FirstElement, 0u);

    uav->Release();
    buf->Release();
}

TEST_F(BufferUAVTests, TypedBuffer_WithOffset)
{
    D3D11_BUFFER_DESC bufDesc = {};
    bufDesc.ByteWidth = 512;
    bufDesc.Usage     = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format              = DXGI_FORMAT_R32_FLOAT;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 16;
    uavDesc.Buffer.NumElements  = 48;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC out = {};
    uav->GetDesc(&out);
    EXPECT_EQ(out.Buffer.FirstElement, 16u);
    EXPECT_EQ(out.Buffer.NumElements, 48u);

    uav->Release();
    buf->Release();
}
