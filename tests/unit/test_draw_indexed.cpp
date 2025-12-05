#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <cstring>
#include "resources/texture2d.h"

using namespace d3d11sw;

struct DrawIndexedTests : ::testing::Test
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

TEST_F(DrawIndexedTests, IndexBufferCreation_R16)
{
    UINT16 indices[] = {0, 1, 2, 2, 1, 3};

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(indices);
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = indices;

    ID3D11Buffer* ib = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, &init, &ib)));

    D3D11_BUFFER_DESC outDesc = {};
    ib->GetDesc(&outDesc);
    EXPECT_EQ(outDesc.ByteWidth, sizeof(indices));
    EXPECT_EQ(outDesc.BindFlags, static_cast<UINT>(D3D11_BIND_INDEX_BUFFER));

    ib->Release();
}

TEST_F(DrawIndexedTests, IndexBufferCreation_R32)
{
    UINT32 indices[] = {0, 1, 2, 3, 4, 5};

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(indices);
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = indices;

    ID3D11Buffer* ib = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, &init, &ib)));

    D3D11_BUFFER_DESC outDesc = {};
    ib->GetDesc(&outDesc);
    EXPECT_EQ(outDesc.ByteWidth, sizeof(indices));

    ib->Release();
}

TEST_F(DrawIndexedTests, IndexBufferBinding)
{
    UINT16 indices[] = {0, 1, 2};

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(indices);
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = indices;

    ID3D11Buffer* ib = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, &init, &ib)));

    context->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);

    ID3D11Buffer*  outBuf    = nullptr;
    DXGI_FORMAT    outFormat = DXGI_FORMAT_UNKNOWN;
    UINT           outOffset = 99;
    context->IAGetIndexBuffer(&outBuf, &outFormat, &outOffset);

    EXPECT_EQ(outBuf, ib);
    EXPECT_EQ(outFormat, DXGI_FORMAT_R16_UINT);
    EXPECT_EQ(outOffset, 0u);

    if (outBuf) { outBuf->Release(); }
    ib->Release();
}

TEST_F(DrawIndexedTests, IndexBufferWithOffset)
{
    UINT32 indices[] = {10, 20, 30, 0, 1, 2};

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(indices);
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = indices;

    ID3D11Buffer* ib = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, &init, &ib)));

    context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 12);

    DXGI_FORMAT outFormat = DXGI_FORMAT_UNKNOWN;
    UINT        outOffset = 0;
    context->IAGetIndexBuffer(nullptr, &outFormat, &outOffset);

    EXPECT_EQ(outFormat, DXGI_FORMAT_R32_UINT);
    EXPECT_EQ(outOffset, 12u);

    ib->Release();
}

TEST_F(DrawIndexedTests, IndexBuffer_NullUnbinds)
{
    UINT16 indices[] = {0, 1, 2};

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(indices);
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = indices;

    ID3D11Buffer* ib = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&desc, &init, &ib)));

    context->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

    ID3D11Buffer* outBuf = nullptr;
    context->IAGetIndexBuffer(&outBuf, nullptr, nullptr);
    EXPECT_EQ(outBuf, nullptr);

    ib->Release();
}

TEST_F(DrawIndexedTests, VertexAndIndexBufferCombination)
{
    float verts[] = {
        0.f, 0.5f, 0.5f,
       -0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, 0.5f,
        0.f, -0.8f, 0.5f,
    };
    UINT16 indices[] = {0, 1, 2, 0, 2, 3};

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(verts);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbInit = {};
    vbInit.pSysMem = verts;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.Usage     = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibInit = {};
    ibInit.pSysMem = indices;

    ID3D11Buffer* ib = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&ibDesc, &ibInit, &ib)));

    UINT stride = 12, offset = 0;
    context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    context->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);

    ID3D11Buffer* outVb = nullptr;
    UINT outStride = 0, outOffset = 0;
    context->IAGetVertexBuffers(0, 1, &outVb, &outStride, &outOffset);
    EXPECT_EQ(outVb, vb);
    EXPECT_EQ(outStride, 12u);

    ID3D11Buffer* outIb = nullptr;
    DXGI_FORMAT outFmt = DXGI_FORMAT_UNKNOWN;
    context->IAGetIndexBuffer(&outIb, &outFmt, nullptr);
    EXPECT_EQ(outIb, ib);
    EXPECT_EQ(outFmt, DXGI_FORMAT_R16_UINT);

    if (outVb) { outVb->Release(); }
    if (outIb) { outIb->Release(); }
    ib->Release();
    vb->Release();
}
