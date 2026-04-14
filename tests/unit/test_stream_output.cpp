#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <vector>
#include <cstring>
#include <fstream>

static std::vector<uint8_t> ReadFile(const char* path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) { return {}; }
    auto sz = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> buf(sz);
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    return buf;
}

struct SOTests : ::testing::Test
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

TEST_F(SOTests, CaptureAndReadBack)
{
    // Input: 3 vertices with known positions and colors
    float vertices[] = {
        // pos (x,y,z)          color (r,g,b,a)
         0.0f,  0.5f, 0.5f,    1.f, 0.f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f,    0.f, 1.f, 0.f, 1.f,
         0.5f, -0.5f, 0.5f,    0.f, 0.f, 1.f, 1.f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    auto vsBytecode = ReadFile(D3D11SW_SHADER_DIR "/vs_color.o");
    ASSERT_FALSE(vsBytecode.empty());

    ID3D11InputLayout* inputLayout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layoutDesc, 2, vsBytecode.data(), vsBytecode.size(), &inputLayout)));

    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
        vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));

    // Create GS with stream output: capture SV_Position (4 floats) + COLOR (4 floats) = 32 bytes/vertex
    auto gsBytecode = ReadFile(D3D11SW_SHADER_DIR "/gs_passthrough.o");
    ASSERT_FALSE(gsBytecode.empty()) << "gs_passthrough.o not found — run compile.sh";

    D3D11_SO_DECLARATION_ENTRY soDecl[] = {
        { 0, "SV_Position", 0, 0, 4, 0 },
        { 0, "COLOR",       0, 0, 4, 0 },
    };
    UINT soStride = 32; // 4+4 floats * 4 bytes

    ID3D11GeometryShader* gs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateGeometryShaderWithStreamOutput(
        gsBytecode.data(), gsBytecode.size(),
        soDecl, 2,
        &soStride, 1,
        0, nullptr, &gs)));

    // Create SO target buffer (big enough for 3 vertices * 32 bytes)
    D3D11_BUFFER_DESC soDesc{};
    soDesc.ByteWidth = 3 * 32;
    soDesc.Usage     = D3D11_USAGE_DEFAULT;
    soDesc.BindFlags = D3D11_BIND_STREAM_OUTPUT;

    ID3D11Buffer* soBuf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&soDesc, nullptr, &soBuf)));

    // Create a dummy render target (required for Draw to proceed)
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = 8; texDesc.Height = 8;
    texDesc.MipLevels = 1; texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* rtTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &rtTex)));
    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex, nullptr, &rtv)));

    auto psBytecode = ReadFile(D3D11SW_SHADER_DIR "/ps_color.o");
    ASSERT_FALSE(psBytecode.empty());
    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
        psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    // Set up pipeline
    context->IASetInputLayout(inputLayout);
    UINT stride = 28, offset = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs, nullptr, 0);
    context->GSSetShader(gs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT viewport{};
    viewport.Width = 8; viewport.Height = 8;
    viewport.MinDepth = 0; viewport.MaxDepth = 1;
    context->RSSetViewports(1, &viewport);

    UINT soOffset = 0;
    context->SOSetTargets(1, reinterpret_cast<ID3D11Buffer**>(&soBuf), &soOffset);

    // Draw
    context->Draw(3, 0);

    // Read back SO buffer
    D3D11_BUFFER_DESC stagingDesc = soDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, soBuf);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    const float* data = static_cast<const float*>(mapped.pData);

    // Vertex 0: pos=(0, 0.5, 0.5, 1), color=(1, 0, 0, 1)
    EXPECT_FLOAT_EQ(data[0],  0.0f);   // pos.x
    EXPECT_FLOAT_EQ(data[1],  0.5f);   // pos.y
    EXPECT_FLOAT_EQ(data[2],  0.5f);   // pos.z
    EXPECT_FLOAT_EQ(data[3],  1.0f);   // pos.w
    EXPECT_FLOAT_EQ(data[4],  1.0f);   // color.r
    EXPECT_FLOAT_EQ(data[5],  0.0f);   // color.g
    EXPECT_FLOAT_EQ(data[6],  0.0f);   // color.b
    EXPECT_FLOAT_EQ(data[7],  1.0f);   // color.a

    // Vertex 1: pos=(-0.5, -0.5, 0.5, 1), color=(0, 1, 0, 1)
    EXPECT_FLOAT_EQ(data[8],  -0.5f);
    EXPECT_FLOAT_EQ(data[9],  -0.5f);
    EXPECT_FLOAT_EQ(data[10],  0.5f);
    EXPECT_FLOAT_EQ(data[11],  1.0f);
    EXPECT_FLOAT_EQ(data[12],  0.0f);
    EXPECT_FLOAT_EQ(data[13],  1.0f);
    EXPECT_FLOAT_EQ(data[14],  0.0f);
    EXPECT_FLOAT_EQ(data[15],  1.0f);

    // Vertex 2: pos=(0.5, -0.5, 0.5, 1), color=(0, 0, 1, 1)
    EXPECT_FLOAT_EQ(data[16],  0.5f);
    EXPECT_FLOAT_EQ(data[17], -0.5f);
    EXPECT_FLOAT_EQ(data[18],  0.5f);
    EXPECT_FLOAT_EQ(data[19],  1.0f);
    EXPECT_FLOAT_EQ(data[20],  0.0f);
    EXPECT_FLOAT_EQ(data[21],  0.0f);
    EXPECT_FLOAT_EQ(data[22],  1.0f);
    EXPECT_FLOAT_EQ(data[23],  1.0f);

    context->Unmap(staging, 0);

    staging->Release();
    soBuf->Release();
    rtv->Release();
    rtTex->Release();
    ps->Release();
    gs->Release();
    vs->Release();
    inputLayout->Release();
    vb->Release();
}

TEST_F(SOTests, DrawAuto)
{
    const UINT W = 64, H = 64;

    // --- Pass 1: GS captures triangle to SO buffer ---

    float vertices[] = {
         0.0f,  0.5f, 0.5f,   1.f, 0.f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f,   1.f, 0.f, 0.f, 1.f,
         0.5f, -0.5f, 0.5f,   1.f, 0.f, 0.f, 1.f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    D3D11_INPUT_ELEMENT_DESC layout1[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    auto vsBytecode = ReadFile(D3D11SW_SHADER_DIR "/vs_color.o");
    ASSERT_FALSE(vsBytecode.empty());

    ID3D11InputLayout* inputLayout1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layout1, 2, vsBytecode.data(), vsBytecode.size(), &inputLayout1)));

    ID3D11VertexShader* vs1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
        vsBytecode.data(), vsBytecode.size(), nullptr, &vs1)));

    auto gsBytecode = ReadFile(D3D11SW_SHADER_DIR "/gs_passthrough.o");
    ASSERT_FALSE(gsBytecode.empty());

    // SO layout: pos(xyzw) + color(rgba) = 32 bytes/vert
    D3D11_SO_DECLARATION_ENTRY soDecl[] = {
        { 0, "SV_Position", 0, 0, 4, 0 },
        { 0, "COLOR",       0, 0, 4, 0 },
    };
    UINT soStride = 32;

    ID3D11GeometryShader* gs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateGeometryShaderWithStreamOutput(
        gsBytecode.data(), gsBytecode.size(),
        soDecl, 2, &soStride, 1, 0, nullptr, &gs)));

    // SO buffer: 3 verts * 32 bytes
    D3D11_BUFFER_DESC soDesc{};
    soDesc.ByteWidth = 3 * 32;
    soDesc.Usage     = D3D11_USAGE_DEFAULT;
    soDesc.BindFlags = D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_VERTEX_BUFFER;

    ID3D11Buffer* soBuf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&soDesc, nullptr, &soBuf)));

    // Dummy RT for pass 1
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

    auto psBytecode = ReadFile(D3D11SW_SHADER_DIR "/ps_color.o");
    ASSERT_FALSE(psBytecode.empty());
    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
        psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    // Pass 1: draw to SO
    context->IASetInputLayout(inputLayout1);
    UINT stride1 = 28, offset1 = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride1, &offset1);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs1, nullptr, 0);
    context->GSSetShader(gs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<FLOAT>(W); viewport.Height = static_cast<FLOAT>(H);
    viewport.MinDepth = 0; viewport.MaxDepth = 1;
    context->RSSetViewports(1, &viewport);

    UINT soOffset = 0;
    context->SOSetTargets(1, reinterpret_cast<ID3D11Buffer**>(&soBuf), &soOffset);
    context->Draw(3, 0);

    // --- Pass 2: DrawAuto from SO buffer ---

    // Unbind SO
    ID3D11Buffer* nullBuf = nullptr;
    UINT nullOffset = 0;
    context->SOSetTargets(1, &nullBuf, &nullOffset);

    // Unbind GS
    context->GSSetShader(nullptr, nullptr, 0);

    // Clear RT
    FLOAT clearColor[4] = {0.f, 0.f, 0.f, 1.f};
    context->ClearRenderTargetView(rtv, clearColor);

    // SO buffer layout is {float4 pos, float4 color} = 32 bytes
    // Use vs_color which expects {float3 pos, float4 color} = 28 bytes
    // But SO wrote float4 pos (xyzw). We need a VS that reads float4 pos.
    // Use vs_passthrough (float3 pos) won't work because stride is 32.
    // Instead, create an input layout matching the SO output: float4 pos + float4 color
    D3D11_INPUT_ELEMENT_DESC layout2[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    // vs_color expects float3 POSITION but we're feeding float4.
    // R32G32B32A32_FLOAT will give 4 components; the VS reads .xyz and ignores .w.
    // vs_color: o.pos = float4(inp.pos, 1.0) — but inp.pos is float3, so it reads .xyz from the input.
    // With R32G32B32A32 input, the IA delivers 4 floats to v[posReg]; vs_color reads .xyz = correct pos.
    ID3D11InputLayout* inputLayout2 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layout2, 2, vsBytecode.data(), vsBytecode.size(), &inputLayout2)));

    context->IASetInputLayout(inputLayout2);
    UINT stride2 = 32, offset2 = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&soBuf), &stride2, &offset2);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs1, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->OMSetRenderTargets(1, &rtv, nullptr);

    context->DrawAuto();

    // Read back RT
    D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, rtTex);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    int redPixels = 0;
    for (UINT y = 0; y < H; ++y)
    {
        const UINT8* row = static_cast<const UINT8*>(mapped.pData) + y * mapped.RowPitch;
        for (UINT x = 0; x < W; ++x)
        {
            float r = row[x * 4 + 0] / 255.f;
            float g = row[x * 4 + 1] / 255.f;
            float b = row[x * 4 + 2] / 255.f;
            if (r > 0.8f && g < 0.2f && b < 0.2f)
            {
                ++redPixels;
            }
        }
    }

    context->Unmap(staging, 0);

    EXPECT_GT(redPixels, 100) << "DrawAuto should render a red triangle from SO buffer";

    staging->Release();
    inputLayout2->Release();
    soBuf->Release();
    rtv->Release();
    rtTex->Release();
    ps->Release();
    gs->Release();
    vs1->Release();
    inputLayout1->Release();
    vb->Release();
}

TEST_F(SOTests, SOWithoutGS)
{
    // VS outputs pos+color directly to SO buffer, no GS shader bytecode
    float vertices[] = {
         0.0f,  0.5f, 0.5f,   1.f, 0.f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f,   0.f, 1.f, 0.f, 1.f,
         0.5f, -0.5f, 0.5f,   0.f, 0.f, 1.f, 1.f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    auto vsBytecode = ReadFile(D3D11SW_SHADER_DIR "/vs_color.o");
    ASSERT_FALSE(vsBytecode.empty());

    ID3D11InputLayout* inputLayout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layoutDesc, 2, vsBytecode.data(), vsBytecode.size(), &inputLayout)));

    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
        vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));

    // Create GS with SO but NO shader bytecode (null passthrough)
    D3D11_SO_DECLARATION_ENTRY soDecl[] = {
        { 0, "SV_Position", 0, 0, 4, 0 },
        { 0, "COLOR",       0, 0, 4, 0 },
    };
    UINT soStride = 32;

    ID3D11GeometryShader* gs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateGeometryShaderWithStreamOutput(
        nullptr, 0,
        soDecl, 2, &soStride, 1, 0, nullptr, &gs)));

    D3D11_BUFFER_DESC soDesc{};
    soDesc.ByteWidth = 3 * 32;
    soDesc.Usage     = D3D11_USAGE_DEFAULT;
    soDesc.BindFlags = D3D11_BIND_STREAM_OUTPUT;

    ID3D11Buffer* soBuf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&soDesc, nullptr, &soBuf)));

    // Dummy RT
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = 8; texDesc.Height = 8;
    texDesc.MipLevels = 1; texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* rtTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &rtTex)));
    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex, nullptr, &rtv)));

    auto psBytecode = ReadFile(D3D11SW_SHADER_DIR "/ps_color.o");
    ASSERT_FALSE(psBytecode.empty());
    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
        psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    context->IASetInputLayout(inputLayout);
    UINT stride = 28, offset = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs, nullptr, 0);
    context->GSSetShader(gs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT viewport{};
    viewport.Width = 8; viewport.Height = 8;
    viewport.MinDepth = 0; viewport.MaxDepth = 1;
    context->RSSetViewports(1, &viewport);

    UINT soOffset = 0;
    context->SOSetTargets(1, reinterpret_cast<ID3D11Buffer**>(&soBuf), &soOffset);
    context->Draw(3, 0);

    // Read back SO buffer
    D3D11_BUFFER_DESC stagingDesc = soDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging)));
    context->CopyResource(staging, soBuf);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    const float* data = static_cast<const float*>(mapped.pData);

    // Vertex 0: pos=(0, 0.5, 0.5, 1), color=(1, 0, 0, 1)
    EXPECT_FLOAT_EQ(data[0],  0.0f);
    EXPECT_FLOAT_EQ(data[1],  0.5f);
    EXPECT_FLOAT_EQ(data[2],  0.5f);
    EXPECT_FLOAT_EQ(data[3],  1.0f);
    EXPECT_FLOAT_EQ(data[4],  1.0f);
    EXPECT_FLOAT_EQ(data[5],  0.0f);
    EXPECT_FLOAT_EQ(data[6],  0.0f);
    EXPECT_FLOAT_EQ(data[7],  1.0f);

    // Vertex 1: pos=(-0.5, -0.5, 0.5, 1), color=(0, 1, 0, 1)
    EXPECT_FLOAT_EQ(data[8],  -0.5f);
    EXPECT_FLOAT_EQ(data[9],  -0.5f);
    EXPECT_FLOAT_EQ(data[10],  0.5f);
    EXPECT_FLOAT_EQ(data[11],  1.0f);
    EXPECT_FLOAT_EQ(data[12],  0.0f);
    EXPECT_FLOAT_EQ(data[13],  1.0f);
    EXPECT_FLOAT_EQ(data[14],  0.0f);
    EXPECT_FLOAT_EQ(data[15],  1.0f);

    // Vertex 2: pos=(0.5, -0.5, 0.5, 1), color=(0, 0, 1, 1)
    EXPECT_FLOAT_EQ(data[16],  0.5f);
    EXPECT_FLOAT_EQ(data[17], -0.5f);
    EXPECT_FLOAT_EQ(data[18],  0.5f);
    EXPECT_FLOAT_EQ(data[19],  1.0f);
    EXPECT_FLOAT_EQ(data[20],  0.0f);
    EXPECT_FLOAT_EQ(data[21],  0.0f);
    EXPECT_FLOAT_EQ(data[22],  1.0f);
    EXPECT_FLOAT_EQ(data[23],  1.0f);

    context->Unmap(staging, 0);

    staging->Release();
    soBuf->Release();
    rtv->Release();
    rtTex->Release();
    ps->Release();
    gs->Release();
    vs->Release();
    inputLayout->Release();
    vb->Release();
}

TEST_F(SOTests, SOWithoutGS_DrawAuto)
{
    const UINT W = 64, H = 64;

    float vertices[] = {
         0.0f,  0.5f, 0.5f,   0.f, 1.f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f,   0.f, 1.f, 0.f, 1.f,
         0.5f, -0.5f, 0.5f,   0.f, 1.f, 0.f, 1.f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    D3D11_INPUT_ELEMENT_DESC layout1[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    auto vsBytecode = ReadFile(D3D11SW_SHADER_DIR "/vs_color.o");
    ASSERT_FALSE(vsBytecode.empty());

    ID3D11InputLayout* inputLayout1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layout1, 2, vsBytecode.data(), vsBytecode.size(), &inputLayout1)));

    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
        vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));

    D3D11_SO_DECLARATION_ENTRY soDecl[] = {
        { 0, "SV_Position", 0, 0, 4, 0 },
        { 0, "COLOR",       0, 0, 4, 0 },
    };
    UINT soStride = 32;

    ID3D11GeometryShader* gs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateGeometryShaderWithStreamOutput(
        nullptr, 0,
        soDecl, 2, &soStride, 1, 0, nullptr, &gs)));

    D3D11_BUFFER_DESC soDesc{};
    soDesc.ByteWidth = 3 * 32;
    soDesc.Usage     = D3D11_USAGE_DEFAULT;
    soDesc.BindFlags = D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_VERTEX_BUFFER;

    ID3D11Buffer* soBuf = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&soDesc, nullptr, &soBuf)));

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

    auto psBytecode = ReadFile(D3D11SW_SHADER_DIR "/ps_color.o");
    ASSERT_FALSE(psBytecode.empty());
    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
        psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    // Pass 1: VS -> SO (no GS bytecode)
    context->IASetInputLayout(inputLayout1);
    UINT stride1 = 28, offset1 = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride1, &offset1);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs, nullptr, 0);
    context->GSSetShader(gs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<FLOAT>(W); viewport.Height = static_cast<FLOAT>(H);
    viewport.MinDepth = 0; viewport.MaxDepth = 1;
    context->RSSetViewports(1, &viewport);

    UINT soOffset = 0;
    context->SOSetTargets(1, reinterpret_cast<ID3D11Buffer**>(&soBuf), &soOffset);
    context->Draw(3, 0);

    // Pass 2: DrawAuto from SO buffer
    ID3D11Buffer* nullBuf = nullptr;
    UINT nullOffset = 0;
    context->SOSetTargets(1, &nullBuf, &nullOffset);
    context->GSSetShader(nullptr, nullptr, 0);

    FLOAT clearColor[4] = {0.f, 0.f, 0.f, 1.f};
    context->ClearRenderTargetView(rtv, clearColor);

    D3D11_INPUT_ELEMENT_DESC layout2[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    ID3D11InputLayout* inputLayout2 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layout2, 2, vsBytecode.data(), vsBytecode.size(), &inputLayout2)));

    context->IASetInputLayout(inputLayout2);
    UINT stride2 = 32, offset2 = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&soBuf), &stride2, &offset2);
    context->DrawAuto();

    D3D11_TEXTURE2D_DESC stagingTexDesc = texDesc;
    stagingTexDesc.Usage          = D3D11_USAGE_STAGING;
    stagingTexDesc.BindFlags      = 0;
    stagingTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* staging = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&stagingTexDesc, nullptr, &staging)));
    context->CopyResource(staging, rtTex);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)));

    int greenPixels = 0;
    for (UINT y = 0; y < H; ++y)
    {
        const UINT8* row = static_cast<const UINT8*>(mapped.pData) + y * mapped.RowPitch;
        for (UINT x = 0; x < W; ++x)
        {
            float r = row[x * 4 + 0] / 255.f;
            float g = row[x * 4 + 1] / 255.f;
            float b = row[x * 4 + 2] / 255.f;
            if (g > 0.8f && r < 0.2f && b < 0.2f)
            {
                ++greenPixels;
            }
        }
    }

    context->Unmap(staging, 0);

    EXPECT_GT(greenPixels, 100) << "DrawAuto from SO-without-GS should render a green triangle";

    staging->Release();
    inputLayout2->Release();
    soBuf->Release();
    rtv->Release();
    rtTex->Release();
    ps->Release();
    gs->Release();
    vs->Release();
    inputLayout1->Release();
    vb->Release();
}

TEST_F(SOTests, MultiStream)
{
    float vertices[] = {
         0.0f,  0.5f, 0.5f,   1.f, 0.f, 0.f, 1.f,
        -0.5f, -0.5f, 0.5f,   0.f, 1.f, 0.f, 1.f,
         0.5f, -0.5f, 0.5f,   0.f, 0.f, 1.f, 1.f,
    };

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage     = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbInit{};
    vbInit.pSysMem = vertices;

    ID3D11Buffer* vb = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    auto vsBytecode = ReadFile(D3D11SW_SHADER_DIR "/vs_color.o");
    ASSERT_FALSE(vsBytecode.empty());

    ID3D11InputLayout* inputLayout = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(
        layoutDesc, 2, vsBytecode.data(), vsBytecode.size(), &inputLayout)));

    ID3D11VertexShader* vs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(
        vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));

    auto gsBytecode = ReadFile(D3D11SW_SHADER_DIR "/gs_multi_stream.o");
    ASSERT_FALSE(gsBytecode.empty()) << "gs_multi_stream.o not found — run compile.sh";

    // Stream 0 → slot 0 (positions, 16 bytes/vert)
    // Stream 1 → slot 1 (colors, 16 bytes/vert)
    D3D11_SO_DECLARATION_ENTRY soDecl[] = {
        { 0, "SV_Position", 0, 0, 4, 0 },
        { 1, "COLOR",       0, 0, 4, 1 },
    };
    UINT soStrides[2] = { 16, 16 };

    ID3D11GeometryShader* gs = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateGeometryShaderWithStreamOutput(
        gsBytecode.data(), gsBytecode.size(),
        soDecl, 2, soStrides, 2,
        D3D11_SO_NO_RASTERIZED_STREAM,
        nullptr, &gs)));

    // Two SO buffers: 3 verts * 16 bytes each
    D3D11_BUFFER_DESC soDesc{};
    soDesc.ByteWidth = 3 * 16;
    soDesc.Usage     = D3D11_USAGE_DEFAULT;
    soDesc.BindFlags = D3D11_BIND_STREAM_OUTPUT;

    ID3D11Buffer* soBuf0 = nullptr;
    ID3D11Buffer* soBuf1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&soDesc, nullptr, &soBuf0)));
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&soDesc, nullptr, &soBuf1)));

    // Dummy RT
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = 8; texDesc.Height = 8;
    texDesc.MipLevels = 1; texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ID3D11Texture2D* rtTex = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &rtTex)));
    ID3D11RenderTargetView* rtv = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex, nullptr, &rtv)));

    auto psBytecode = ReadFile(D3D11SW_SHADER_DIR "/ps_color.o");
    ASSERT_FALSE(psBytecode.empty());
    ID3D11PixelShader* ps = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(
        psBytecode.data(), psBytecode.size(), nullptr, &ps)));

    context->IASetInputLayout(inputLayout);
    UINT stride = 28, offset = 0;
    context->IASetVertexBuffers(0, 1, reinterpret_cast<ID3D11Buffer**>(&vb), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vs, nullptr, 0);
    context->GSSetShader(gs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT viewport{};
    viewport.Width = 8; viewport.Height = 8;
    viewport.MinDepth = 0; viewport.MaxDepth = 1;
    context->RSSetViewports(1, &viewport);

    ID3D11Buffer* soBuffers[2] = { soBuf0, soBuf1 };
    UINT soOffsets[2] = { 0, 0 };
    context->SOSetTargets(2, reinterpret_cast<ID3D11Buffer**>(soBuffers), soOffsets);

    context->Draw(3, 0);

    // Read back stream 0 (positions)
    D3D11_BUFFER_DESC stagingDesc = soDesc;
    stagingDesc.Usage          = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags      = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* staging0 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging0)));
    context->CopyResource(staging0, soBuf0);

    D3D11_MAPPED_SUBRESOURCE mapped0{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging0, 0, D3D11_MAP_READ, 0, &mapped0)));
    const float* pos = static_cast<const float*>(mapped0.pData);

    // Stream 0, Vertex 0: pos=(0, 0.5, 0.5, 1)
    EXPECT_FLOAT_EQ(pos[0],  0.0f);
    EXPECT_FLOAT_EQ(pos[1],  0.5f);
    EXPECT_FLOAT_EQ(pos[2],  0.5f);
    EXPECT_FLOAT_EQ(pos[3],  1.0f);
    // Stream 0, Vertex 1: pos=(-0.5, -0.5, 0.5, 1)
    EXPECT_FLOAT_EQ(pos[4], -0.5f);
    EXPECT_FLOAT_EQ(pos[5], -0.5f);
    EXPECT_FLOAT_EQ(pos[6],  0.5f);
    EXPECT_FLOAT_EQ(pos[7],  1.0f);
    // Stream 0, Vertex 2: pos=(0.5, -0.5, 0.5, 1)
    EXPECT_FLOAT_EQ(pos[8],  0.5f);
    EXPECT_FLOAT_EQ(pos[9], -0.5f);
    EXPECT_FLOAT_EQ(pos[10], 0.5f);
    EXPECT_FLOAT_EQ(pos[11], 1.0f);

    context->Unmap(staging0, 0);

    // Read back stream 1 (colors)
    ID3D11Buffer* staging1 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&stagingDesc, nullptr, &staging1)));
    context->CopyResource(staging1, soBuf1);

    D3D11_MAPPED_SUBRESOURCE mapped1{};
    ASSERT_TRUE(SUCCEEDED(context->Map(staging1, 0, D3D11_MAP_READ, 0, &mapped1)));
    const float* col = static_cast<const float*>(mapped1.pData);

    // Stream 1, Vertex 0: color=(1, 0, 0, 1)
    EXPECT_FLOAT_EQ(col[0], 1.0f);
    EXPECT_FLOAT_EQ(col[1], 0.0f);
    EXPECT_FLOAT_EQ(col[2], 0.0f);
    EXPECT_FLOAT_EQ(col[3], 1.0f);
    // Stream 1, Vertex 1: color=(0, 1, 0, 1)
    EXPECT_FLOAT_EQ(col[4], 0.0f);
    EXPECT_FLOAT_EQ(col[5], 1.0f);
    EXPECT_FLOAT_EQ(col[6], 0.0f);
    EXPECT_FLOAT_EQ(col[7], 1.0f);
    // Stream 1, Vertex 2: color=(0, 0, 1, 1)
    EXPECT_FLOAT_EQ(col[8],  0.0f);
    EXPECT_FLOAT_EQ(col[9],  0.0f);
    EXPECT_FLOAT_EQ(col[10], 1.0f);
    EXPECT_FLOAT_EQ(col[11], 1.0f);

    context->Unmap(staging1, 0);

    staging1->Release();
    staging0->Release();
    soBuf1->Release();
    soBuf0->Release();
    rtv->Release();
    rtTex->Release();
    ps->Release();
    gs->Release();
    vs->Release();
    inputLayout->Release();
    vb->Release();
}
