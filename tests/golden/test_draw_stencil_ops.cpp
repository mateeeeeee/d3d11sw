#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "golden_test_util.h"

struct DrawGoldenTests : ::testing::Test
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

TEST_F(DrawGoldenTests, StencilDecrSat)
{
    const UINT W = 64, H = 64;

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = W; texDesc.Height = H;
    texDesc.MipLevels = 1; texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* rtTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &rtTex)));
    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex, nullptr, &rtv)));

    D3D11_TEXTURE2D_DESC dsTexDesc{};
    dsTexDesc.Width = W; dsTexDesc.Height = H;
    dsTexDesc.MipLevels = 1; dsTexDesc.ArraySize = 1;
    dsTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsTexDesc.SampleDesc.Count = 1;
    dsTexDesc.Usage = D3D11_USAGE_DEFAULT;
    dsTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* dsTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&dsTexDesc, nullptr, &dsTex)));
    ID3D11DepthStencilView* dsv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilView(dsTex, nullptr, &dsv)));

    FLOAT clearColor[4] = {0.f, 0.f, 0.f, 1.f};
    context->ClearRenderTargetView(rtv, clearColor);
    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 1);

    float triVerts[] = {
         0.0f,  0.7f, 0.5f,
         0.7f, -0.7f, 0.5f,
        -0.7f, -0.7f, 0.5f,
    };
    float fullVerts[] = {
        -1.0f,  1.0f, 0.5f,
         3.0f,  1.0f, 0.5f,
        -1.0f, -3.0f, 0.5f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(triVerts);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = triVerts;
    ID3D11Buffer* vbTri = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vbTri)));

    vbDesc.ByteWidth = sizeof(fullVerts);
    vbInit.pSysMem = fullVerts;
    ID3D11Buffer* vbFull = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vbFull)));

    float greenColor[4] = {0.f, 1.f, 0.f, 1.f};
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = 16;
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA cbInit{};
    cbInit.pSysMem = greenColor;
    ID3D11Buffer* cbGreen = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInit, &cbGreen)));

    float dummyColor[4] = {0.f, 0.f, 0.f, 0.f};
    D3D11_SUBRESOURCE_DATA cbDummyInit{};
    cbDummyInit.pSysMem = dummyColor;
    ID3D11Buffer* cbDummy = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbDummyInit, &cbDummy)));

    auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_passthrough.o");
    ASSERT_FALSE(vsBytecode.empty());
    auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_solid_cbuffer.o");
    ASSERT_FALSE(psBytecode.empty());

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* il = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(layoutDesc, 1, vsBytecode.data(), vsBytecode.size(), &il)));
    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));
    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    context->IASetInputLayout(il);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->OMSetRenderTargets(1, &rtv, dsv);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<FLOAT>(W);
    viewport.Height = static_cast<FLOAT>(H);
    viewport.MaxDepth = 1.f;
    context->RSSetViewports(1, &viewport);

    D3D11_DEPTH_STENCIL_DESC dsDesc1{};
    dsDesc1.DepthEnable = FALSE;
    dsDesc1.StencilEnable = TRUE;
    dsDesc1.StencilReadMask = 0xFF;
    dsDesc1.StencilWriteMask = 0xFF;
    dsDesc1.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc1.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR_SAT;
    dsDesc1.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc1.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc1.BackFace = dsDesc1.FrontFace;
    ID3D11DepthStencilState* dsState1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilState(&dsDesc1, &dsState1)));

    context->OMSetDepthStencilState(dsState1, 0);
    context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cbDummy));
    UINT stride = 12, offset = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vbTri), &stride, &offset);
    context->Draw(3, 0);

    D3D11_DEPTH_STENCIL_DESC dsDesc2{};
    dsDesc2.DepthEnable = FALSE;
    dsDesc2.StencilEnable = TRUE;
    dsDesc2.StencilReadMask = 0xFF;
    dsDesc2.StencilWriteMask = 0xFF;
    dsDesc2.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
    dsDesc2.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc2.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc2.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc2.BackFace = dsDesc2.FrontFace;
    ID3D11DepthStencilState* dsState2 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilState(&dsDesc2, &dsState2)));

    context->OMSetDepthStencilState(dsState2, 0);
    context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cbGreen));
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vbFull), &stride, &offset);
    context->Draw(3, 0);

    D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, rtTex);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    std::vector<float> pixels(W * H * 4);
    for (UINT y = 0; y < H; ++y)
    {
        const UINT8* row = static_cast<const UINT8*>(mapped.pData) + y * mapped.RowPitch;
        for (UINT x = 0; x < W; ++x)
        {
            UINT idx = (y * W + x) * 4;
            pixels[idx + 0] = row[x * 4 + 0] / 255.f;
            pixels[idx + 1] = row[x * 4 + 1] / 255.f;
            pixels[idx + 2] = row[x * 4 + 2] / 255.f;
            pixels[idx + 3] = row[x * 4 + 3] / 255.f;
        }
    }
    context->Unmap(staging, 0);

    auto result = CheckGolden("draw_stencil_decr_sat_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    int greenPx = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (pixels[i*4+1] > 0.9f && pixels[i*4+0] < 0.1f) { ++greenPx; }
    }
    EXPECT_GT(greenPx, 100) << "Expected green pixels in DECR_SAT stencil area";

    staging->Release();
    dsState2->Release(); dsState1->Release();
    ps->Release(); vs->Release(); il->Release();
    cbGreen->Release(); cbDummy->Release();
    vbFull->Release(); vbTri->Release();
    dsv->Release(); dsTex->Release(); rtv->Release(); rtTex->Release();
}

TEST_F(DrawGoldenTests, StencilInvert)
{
    const UINT W = 64, H = 64;

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = W; texDesc.Height = H;
    texDesc.MipLevels = 1; texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* rtTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &rtTex)));
    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex, nullptr, &rtv)));

    D3D11_TEXTURE2D_DESC dsTexDesc{};
    dsTexDesc.Width = W; dsTexDesc.Height = H;
    dsTexDesc.MipLevels = 1; dsTexDesc.ArraySize = 1;
    dsTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsTexDesc.SampleDesc.Count = 1;
    dsTexDesc.Usage = D3D11_USAGE_DEFAULT;
    dsTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* dsTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&dsTexDesc, nullptr, &dsTex)));
    ID3D11DepthStencilView* dsv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilView(dsTex, nullptr, &dsv)));

    FLOAT clearColor[4] = {0.f, 0.f, 0.f, 1.f};
    context->ClearRenderTargetView(rtv, clearColor);
    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    float triVerts[] = {
         0.0f,  0.7f, 0.5f,
         0.7f, -0.7f, 0.5f,
        -0.7f, -0.7f, 0.5f,
    };
    float fullVerts[] = {
        -1.0f,  1.0f, 0.5f,
         3.0f,  1.0f, 0.5f,
        -1.0f, -3.0f, 0.5f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(triVerts);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = triVerts;
    ID3D11Buffer* vbTri = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vbTri)));

    vbDesc.ByteWidth = sizeof(fullVerts);
    vbInit.pSysMem = fullVerts;
    ID3D11Buffer* vbFull = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vbFull)));

    float yellowColor[4] = {1.f, 1.f, 0.f, 1.f};
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = 16;
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA cbInit{};
    cbInit.pSysMem = yellowColor;
    ID3D11Buffer* cbYellow = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbInit, &cbYellow)));

    float dummyColor[4] = {0.f, 0.f, 0.f, 0.f};
    D3D11_SUBRESOURCE_DATA cbDummyInit{};
    cbDummyInit.pSysMem = dummyColor;
    ID3D11Buffer* cbDummy = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&cbDesc, &cbDummyInit, &cbDummy)));

    auto vsBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/vs_passthrough.o");
    ASSERT_FALSE(vsBytecode.empty());
    auto psBytecode = ReadBytecode(D3D11SW_SHADER_DIR "/ps_solid_cbuffer.o");
    ASSERT_FALSE(psBytecode.empty());

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ID3D11InputLayout* il = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(layoutDesc, 1, vsBytecode.data(), vsBytecode.size(), &il)));
    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));
    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    context->IASetInputLayout(il);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->OMSetRenderTargets(1, &rtv, dsv);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<FLOAT>(W);
    viewport.Height = static_cast<FLOAT>(H);
    viewport.MaxDepth = 1.f;
    context->RSSetViewports(1, &viewport);

    D3D11_DEPTH_STENCIL_DESC dsDesc1{};
    dsDesc1.DepthEnable = FALSE;
    dsDesc1.StencilEnable = TRUE;
    dsDesc1.StencilReadMask = 0xFF;
    dsDesc1.StencilWriteMask = 0xFF;
    dsDesc1.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc1.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INVERT;
    dsDesc1.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc1.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc1.BackFace = dsDesc1.FrontFace;
    ID3D11DepthStencilState* dsState1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilState(&dsDesc1, &dsState1)));

    context->OMSetDepthStencilState(dsState1, 0);
    context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cbDummy));
    UINT stride = 12, offset = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vbTri), &stride, &offset);
    context->Draw(3, 0);

    D3D11_DEPTH_STENCIL_DESC dsDesc2{};
    dsDesc2.DepthEnable = FALSE;
    dsDesc2.StencilEnable = TRUE;
    dsDesc2.StencilReadMask = 0xFF;
    dsDesc2.StencilWriteMask = 0xFF;
    dsDesc2.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
    dsDesc2.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc2.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc2.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc2.BackFace = dsDesc2.FrontFace;
    ID3D11DepthStencilState* dsState2 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateDepthStencilState(&dsDesc2, &dsState2)));

    context->OMSetDepthStencilState(dsState2, 0);
    context->PSSetConstantBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&cbYellow));
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vbFull), &stride, &offset);
    context->Draw(3, 0);

    D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, rtTex);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    std::vector<float> pixels(W * H * 4);
    for (UINT y = 0; y < H; ++y)
    {
        const UINT8* row = static_cast<const UINT8*>(mapped.pData) + y * mapped.RowPitch;
        for (UINT x = 0; x < W; ++x)
        {
            UINT idx = (y * W + x) * 4;
            pixels[idx + 0] = row[x * 4 + 0] / 255.f;
            pixels[idx + 1] = row[x * 4 + 1] / 255.f;
            pixels[idx + 2] = row[x * 4 + 2] / 255.f;
            pixels[idx + 3] = row[x * 4 + 3] / 255.f;
        }
    }
    context->Unmap(staging, 0);

    auto result = CheckGolden("draw_stencil_invert_64x64", pixels.data(), W, H, 0.0f);
    EXPECT_TRUE(result.passed) << result.message;

    int yellowPx = 0;
    for (UINT i = 0; i < W * H; ++i)
    {
        if (pixels[i*4+0] > 0.9f && pixels[i*4+1] > 0.9f && pixels[i*4+2] < 0.1f) { ++yellowPx; }
    }
    EXPECT_GT(yellowPx, 100) << "Expected yellow pixels in INVERT stencil area";

    staging->Release();
    dsState2->Release(); dsState1->Release();
    ps->Release(); vs->Release(); il->Release();
    cbYellow->Release(); cbDummy->Release();
    vbFull->Release(); vbTri->Release();
    dsv->Release(); dsTex->Release(); rtv->Release(); rtTex->Release();
}
