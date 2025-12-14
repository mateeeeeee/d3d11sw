#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <cstring>

struct ClearUAVTests : ::testing::Test
{
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* context = nullptr;

    void SetUp() override
    {
        D3D_FEATURE_LEVEL fl;
        ASSERT_TRUE(SUCCEEDED(D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION, &device, &fl, &context)));
    }

    void TearDown() override
    {
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }
};

TEST_F(ClearUAVTests, ClearUAVUint_R32)
{
    const UINT N = 16;

    D3D11_BUFFER_DESC bufDesc{};
    bufDesc.ByteWidth = N * 4;
    bufDesc.Usage     = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format              = DXGI_FORMAT_R32_UINT;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements  = N;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

    UINT clearVal[4] = {42, 0, 0, 0};
    context->ClearUnorderedAccessViewUint(uav, clearVal);

    D3D11_BUFFER_DESC stagingDesc = bufDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, buf);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    auto data = static_cast<const UINT*>(mapped.pData);
    for (UINT i = 0; i < N; ++i)
    {
        EXPECT_EQ(data[i], 42u) << "Mismatch at index " << i;
    }

    context->Unmap(staging, 0);
    staging->Release();
    uav->Release();
    buf->Release();
}

TEST_F(ClearUAVTests, ClearUAVFloat_R32)
{
    const UINT N = 16;

    D3D11_BUFFER_DESC bufDesc{};
    bufDesc.ByteWidth = N * 4;
    bufDesc.Usage     = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format              = DXGI_FORMAT_R32_FLOAT;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements  = N;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

    FLOAT clearVal[4] = {3.14f, 0.f, 0.f, 0.f};
    context->ClearUnorderedAccessViewFloat(uav, clearVal);

    D3D11_BUFFER_DESC stagingDesc = bufDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, buf);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    auto data = static_cast<const float*>(mapped.pData);
    for (UINT i = 0; i < N; ++i)
    {
        EXPECT_FLOAT_EQ(data[i], 3.14f) << "Mismatch at index " << i;
    }

    context->Unmap(staging, 0);
    staging->Release();
    uav->Release();
    buf->Release();
}

TEST_F(ClearUAVTests, ClearUAVUint_R32G32B32A32)
{
    const UINT N = 8;

    D3D11_BUFFER_DESC bufDesc{};
    bufDesc.ByteWidth = N * 16;
    bufDesc.Usage     = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    ID3D11Buffer* buf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&bufDesc, nullptr, &buf)));

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format              = DXGI_FORMAT_R32G32B32A32_UINT;
    uavDesc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements  = N;

    ID3D11UnorderedAccessView* uav = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateUnorderedAccessView(buf, &uavDesc, &uav)));

    UINT clearVal[4] = {1, 2, 3, 4};
    context->ClearUnorderedAccessViewUint(uav, clearVal);

    D3D11_BUFFER_DESC stagingDesc = bufDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, buf);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    auto data = static_cast<const UINT*>(mapped.pData);
    for (UINT i = 0; i < N; ++i)
    {
        EXPECT_EQ(data[i * 4 + 0], 1u);
        EXPECT_EQ(data[i * 4 + 1], 2u);
        EXPECT_EQ(data[i * 4 + 2], 3u);
        EXPECT_EQ(data[i * 4 + 3], 4u);
    }

    context->Unmap(staging, 0);
    staging->Release();
    uav->Release();
    buf->Release();
}
