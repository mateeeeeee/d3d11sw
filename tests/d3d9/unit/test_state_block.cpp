#include <gtest/gtest.h>
#include <d3d9.h>
#include <d3d9types.h>
#include <cstring>

namespace
{

IDirect3DDevice9* MakeDevice()
{
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth  = 8;
    pp.BackBufferHeight = 8;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferCount  = 1;
    pp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    pp.Windowed         = TRUE;
    IDirect3DDevice9* dev = nullptr;
    d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, nullptr,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dev);
    d3d->Release();
    return dev;
}

} // namespace

class D3D9StateBlockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _dev = MakeDevice();
        ASSERT_NE(_dev, nullptr);
    }
    void TearDown() override
    {
        if (_dev)
        {
            _dev->Release();
        }
    }
    IDirect3DDevice9* _dev = nullptr;
};

TEST_F(D3D9StateBlockTests, RecordingDoesNotModifyDeviceState)
{
    _dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

    _dev->BeginStateBlock();
    _dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    IDirect3DStateBlock9* sb = nullptr;
    _dev->EndStateBlock(&sb);

    DWORD val = 0;
    _dev->GetRenderState(D3DRS_CULLMODE, &val);
    EXPECT_EQ(val, static_cast<DWORD>(D3DCULL_CCW));

    sb->Release();
}

TEST_F(D3D9StateBlockTests, ApplyOnlyAffectsRecordedStates)
{
    _dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    _dev->SetRenderState(D3DRS_ZENABLE, TRUE);

    _dev->BeginStateBlock();
    _dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    IDirect3DStateBlock9* sb = nullptr;
    _dev->EndStateBlock(&sb);

    _dev->SetRenderState(D3DRS_ZENABLE, FALSE);

    sb->Apply();

    DWORD cull = 0, zenable = 0;
    _dev->GetRenderState(D3DRS_CULLMODE, &cull);
    _dev->GetRenderState(D3DRS_ZENABLE, &zenable);
    EXPECT_EQ(cull, static_cast<DWORD>(D3DCULL_CW));
    EXPECT_EQ(zenable, static_cast<DWORD>(FALSE));

    sb->Release();
}

TEST_F(D3D9StateBlockTests, RecordedTextureDoesNotClobberDevice)
{
    IDirect3DTexture9* tex = nullptr;
    _dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, nullptr);
    _dev->SetTexture(0, tex);

    _dev->BeginStateBlock();
    _dev->SetTexture(0, nullptr);
    IDirect3DStateBlock9* sb = nullptr;
    _dev->EndStateBlock(&sb);

    IDirect3DBaseTexture9* current = nullptr;
    _dev->GetTexture(0, &current);
    EXPECT_EQ(current, static_cast<IDirect3DBaseTexture9*>(tex));
    if (current)
    {
        current->Release();
    }

    sb->Apply();

    _dev->GetTexture(0, &current);
    EXPECT_EQ(current, nullptr);

    sb->Release();
    tex->Release();
}

TEST_F(D3D9StateBlockTests, RecordedConstantsPreserved)
{
    float original[4] = {1.f, 2.f, 3.f, 4.f};
    _dev->SetPixelShaderConstantF(0, original, 1);

    float recorded[4] = {10.f, 20.f, 30.f, 40.f};
    _dev->BeginStateBlock();
    _dev->SetPixelShaderConstantF(0, recorded, 1);
    IDirect3DStateBlock9* sb = nullptr;
    _dev->EndStateBlock(&sb);

    float readBack[4] = {};
    _dev->GetPixelShaderConstantF(0, readBack, 1);
    EXPECT_FLOAT_EQ(readBack[0], 1.f);
    EXPECT_FLOAT_EQ(readBack[1], 2.f);

    sb->Apply();
    _dev->GetPixelShaderConstantF(0, readBack, 1);
    EXPECT_FLOAT_EQ(readBack[0], 10.f);
    EXPECT_FLOAT_EQ(readBack[1], 20.f);

    sb->Release();
}

TEST_F(D3D9StateBlockTests, CaptureReReadsRecordedStatesFromDevice)
{
    _dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

    _dev->BeginStateBlock();
    _dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    IDirect3DStateBlock9* sb = nullptr;
    _dev->EndStateBlock(&sb);

    _dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_POINT);
    sb->Capture();

    _dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    sb->Apply();

    DWORD val = 0;
    _dev->GetRenderState(D3DRS_FILLMODE, &val);
    EXPECT_EQ(val, static_cast<DWORD>(D3DFILL_POINT));

    sb->Release();
}

TEST_F(D3D9StateBlockTests, CreateStateBlockAppliesAllState)
{
    _dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    _dev->SetRenderState(D3DRS_ZENABLE, FALSE);

    IDirect3DStateBlock9* sb = nullptr;
    _dev->CreateStateBlock(D3DSBT_ALL, &sb);

    _dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
    _dev->SetRenderState(D3DRS_ZENABLE, TRUE);

    sb->Apply();

    DWORD cull = 0, zenable = 0;
    _dev->GetRenderState(D3DRS_CULLMODE, &cull);
    _dev->GetRenderState(D3DRS_ZENABLE, &zenable);
    EXPECT_EQ(cull, static_cast<DWORD>(D3DCULL_NONE));
    EXPECT_EQ(zenable, static_cast<DWORD>(FALSE));

    sb->Release();
}

TEST_F(D3D9StateBlockTests, RecordedTransformDoesNotModifyDevice)
{
    D3DMATRIX identity = {};
    identity._11 = identity._22 = identity._33 = identity._44 = 1.f;
    _dev->SetTransform(D3DTS_WORLD, &identity);

    D3DMATRIX custom = {};
    custom._11 = 42.f;
    _dev->BeginStateBlock();
    _dev->SetTransform(D3DTS_WORLD, &custom);
    IDirect3DStateBlock9* sb = nullptr;
    _dev->EndStateBlock(&sb);

    D3DMATRIX readBack = {};
    _dev->GetTransform(D3DTS_WORLD, &readBack);
    EXPECT_FLOAT_EQ(readBack._11, 1.f);

    sb->Apply();
    _dev->GetTransform(D3DTS_WORLD, &readBack);
    EXPECT_FLOAT_EQ(readBack._11, 42.f);

    sb->Release();
}

TEST_F(D3D9StateBlockTests, RecordedViewportSelectiveApply)
{
    D3DVIEWPORT9 vp1 = {0, 0, 8, 8, 0.f, 1.f};
    D3DVIEWPORT9 vp2 = {1, 1, 4, 4, 0.f, 1.f};
    _dev->SetViewport(&vp1);

    _dev->BeginStateBlock();
    _dev->SetViewport(&vp2);
    IDirect3DStateBlock9* sb = nullptr;
    _dev->EndStateBlock(&sb);

    D3DVIEWPORT9 readBack = {};
    _dev->GetViewport(&readBack);
    EXPECT_EQ(readBack.X, 0u);
    EXPECT_EQ(readBack.Width, 8u);

    sb->Apply();
    _dev->GetViewport(&readBack);
    EXPECT_EQ(readBack.X, 1u);
    EXPECT_EQ(readBack.Width, 4u);

    sb->Release();
}

TEST_F(D3D9StateBlockTests, NestedBeginStateBlockFails)
{
    HRESULT hr = _dev->BeginStateBlock();
    EXPECT_EQ(hr, S_OK);

    hr = _dev->BeginStateBlock();
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);

    IDirect3DStateBlock9* sb = nullptr;
    _dev->EndStateBlock(&sb);
    sb->Release();
}

TEST_F(D3D9StateBlockTests, EndStateBlockWithoutBeginFails)
{
    IDirect3DStateBlock9* sb = nullptr;
    HRESULT hr = _dev->EndStateBlock(&sb);
    EXPECT_EQ(hr, D3DERR_INVALIDCALL);
    EXPECT_EQ(sb, nullptr);
}
