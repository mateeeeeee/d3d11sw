#include <gtest/gtest.h>
#include <d3d11_4.h>
#include <cstring>
#include <cmath>

struct VertexFormatTests : ::testing::Test
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

    void DrawAndReadback(DXGI_FORMAT vertFmt, UINT vertStride, const void* vertData, UINT vertSize,
                         float outPixel[4])
    {
        const UINT W = 4, H = 4;

        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = W; texDesc.Height = H;
        texDesc.MipLevels = 1; texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

        ID3D11Texture2D* rtTex = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateTexture2D(&texDesc, nullptr, &rtTex)));
        ID3D11RenderTargetView* rtv = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateRenderTargetView(rtTex, nullptr, &rtv)));

        FLOAT clearColor[4] = {0.f, 0.f, 0.f, 0.f};
        context->ClearRenderTargetView(rtv, clearColor);
        context->OMSetRenderTargets(1, &rtv, nullptr);

        D3D11_VIEWPORT vp{};
        vp.Width = (FLOAT)W; vp.Height = (FLOAT)H; vp.MaxDepth = 1.f;
        context->RSSetViewports(1, &vp);

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = vertSize;
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vbInit{};
        vbInit.pSysMem = vertData;
        ID3D11Buffer* vb = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateBuffer(&vbDesc, &vbInit, &vb)));

        auto vsBytecode = ReadFile(D3D11SW_SHADER_DIR "/vs_color.o");
        ASSERT_FALSE(vsBytecode.empty()) << "vs_color.o not found";
        auto psBytecode = ReadFile(D3D11SW_SHADER_DIR "/ps_color.o");
        ASSERT_FALSE(psBytecode.empty()) << "ps_color.o not found";

        ID3D11VertexShader* vs = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateVertexShader(vsBytecode.data(), vsBytecode.size(), nullptr, &vs)));
        ID3D11PixelShader* ps = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreatePixelShader(psBytecode.data(), psBytecode.size(), nullptr, &ps)));

        D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                          D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, vertFmt,                      0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        ID3D11InputLayout* il = nullptr;
        ASSERT_TRUE(SUCCEEDED(device->CreateInputLayout(layoutDesc, 2, vsBytecode.data(), vsBytecode.size(), &il)));

        context->IASetInputLayout(il);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &vb, &vertStride, &offset);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(vs, nullptr, 0);
        context->PSSetShader(ps, nullptr, 0);
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

        bool found = false;
        for (UINT y = 0; y < H && !found; ++y)
        {
            const float* row = reinterpret_cast<const float*>(
                static_cast<const UINT8*>(mapped.pData) + y * mapped.RowPitch);
            for (UINT x = 0; x < W && !found; ++x)
            {
                const float* px = row + x * 4;
                if (px[0] != 0.f || px[1] != 0.f || px[2] != 0.f || px[3] != 0.f)
                {
                    outPixel[0] = px[0];
                    outPixel[1] = px[1];
                    outPixel[2] = px[2];
                    outPixel[3] = px[3];
                    found = true;
                }
            }
        }
        context->Unmap(staging, 0);

        EXPECT_TRUE(found) << "No non-zero pixel found";

        staging->Release();
        ps->Release(); vs->Release(); il->Release();
        vb->Release(); rtv->Release(); rtTex->Release();
    }

    static std::vector<unsigned char> ReadFile(const char* path)
    {
        std::FILE* f = std::fopen(path, "rb");
        if (!f) { return {}; }
        std::fseek(f, 0, SEEK_END);
        long size = std::ftell(f);
        if (size <= 0) { std::fclose(f); return {}; }
        std::fseek(f, 0, SEEK_SET);
        std::vector<unsigned char> data(static_cast<size_t>(size));
        std::fread(data.data(), 1, data.size(), f);
        std::fclose(f);
        return data;
    }
};

// Fullscreen triangle: covers entire 4x4 RT, all verts have same color
#define MAKE_VERTS(ColorType, colorVal) \
    struct V { float x, y, z; ColorType color; }; \
    V verts[3] = { \
        {-1.f,  3.f, 0.5f, colorVal}, \
        { 3.f, -1.f, 0.5f, colorVal}, \
        {-1.f, -1.f, 0.5f, colorVal}, \
    };

TEST_F(VertexFormatTests, R32G32B32A32_FLOAT)
{
    struct Color { float r, g, b, a; };
    MAKE_VERTS(Color, (Color{0.25f, 0.5f, 0.75f, 1.f}));
    float px[4];
    DrawAndReadback(DXGI_FORMAT_R32G32B32A32_FLOAT, sizeof(V), verts, sizeof(verts), px);
    EXPECT_NEAR(px[0], 0.25f, 1e-5f);
    EXPECT_NEAR(px[1], 0.5f,  1e-5f);
    EXPECT_NEAR(px[2], 0.75f, 1e-5f);
    EXPECT_NEAR(px[3], 1.f,   1e-5f);
}

TEST_F(VertexFormatTests, R8G8B8A8_UNORM)
{
    struct Color { uint8_t r, g, b, a; };
    MAKE_VERTS(Color, (Color{64, 128, 191, 255}));
    float px[4];
    DrawAndReadback(DXGI_FORMAT_R8G8B8A8_UNORM, sizeof(V), verts, sizeof(verts), px);
    EXPECT_NEAR(px[0], 64  / 255.f, 1e-2f);
    EXPECT_NEAR(px[1], 128 / 255.f, 1e-2f);
    EXPECT_NEAR(px[2], 191 / 255.f, 1e-2f);
    EXPECT_NEAR(px[3], 1.f,         1e-2f);
}

TEST_F(VertexFormatTests, R8G8B8A8_SNORM)
{
    struct Color { int8_t r, g, b, a; };
    MAKE_VERTS(Color, (Color{0, 63, 127, -127}));
    float px[4];
    DrawAndReadback(DXGI_FORMAT_R8G8B8A8_SNORM, sizeof(V), verts, sizeof(verts), px);
    EXPECT_NEAR(px[0], 0.f,    1e-2f);
    EXPECT_NEAR(px[1], 63/127.f, 1e-2f);
    EXPECT_NEAR(px[2], 1.f,    1e-2f);
    EXPECT_NEAR(px[3], -1.f,   1e-2f);
}

TEST_F(VertexFormatTests, R10G10B10A2_UNORM)
{
    struct Color { uint32_t packed; };
    uint32_t p = (1023u) | (512u << 10) | (0u << 20) | (3u << 30);
    MAKE_VERTS(Color, (Color{p}));
    float px[4];
    DrawAndReadback(DXGI_FORMAT_R10G10B10A2_UNORM, sizeof(V), verts, sizeof(verts), px);
    EXPECT_NEAR(px[0], 1.f,        1e-2f);
    EXPECT_NEAR(px[1], 512/1023.f, 1e-2f);
    EXPECT_NEAR(px[2], 0.f,        1e-2f);
    EXPECT_NEAR(px[3], 1.f,        1e-2f);
}

TEST_F(VertexFormatTests, R16G16_FLOAT)
{
    auto toHalf = [](float f) -> uint16_t
    {
        uint32_t bits;
        std::memcpy(&bits, &f, 4);
        uint32_t sign = (bits >> 16) & 0x8000;
        int32_t exp = ((bits >> 23) & 0xFF) - 127 + 15;
        uint32_t man = (bits >> 13) & 0x3FF;
        if (exp <= 0) { return static_cast<uint16_t>(sign); }
        if (exp >= 31) { return static_cast<uint16_t>(sign | 0x7C00); }
        return static_cast<uint16_t>(sign | (exp << 10) | man);
    };
    struct Color { uint16_t r, g; };
    MAKE_VERTS(Color, (Color{toHalf(0.5f), toHalf(0.25f)}));
    float px[4];
    DrawAndReadback(DXGI_FORMAT_R16G16_FLOAT, sizeof(V), verts, sizeof(verts), px);
    EXPECT_NEAR(px[0], 0.5f,  1e-3f);
    EXPECT_NEAR(px[1], 0.25f, 1e-3f);
}

TEST_F(VertexFormatTests, R16G16_SNORM)
{
    struct Color { int16_t r, g; };
    MAKE_VERTS(Color, (Color{32767, -32767}));
    float px[4];
    DrawAndReadback(DXGI_FORMAT_R16G16_SNORM, sizeof(V), verts, sizeof(verts), px);
    EXPECT_NEAR(px[0], 1.f,  1e-4f);
    EXPECT_NEAR(px[1], -1.f, 1e-4f);
}

TEST_F(VertexFormatTests, R16G16_UNORM)
{
    struct Color { uint16_t r, g; };
    MAKE_VERTS(Color, (Color{65535, 32768}));
    float px[4];
    DrawAndReadback(DXGI_FORMAT_R16G16_UNORM, sizeof(V), verts, sizeof(verts), px);
    EXPECT_NEAR(px[0], 1.f,          1e-4f);
    EXPECT_NEAR(px[1], 32768/65535.f, 1e-4f);
}
