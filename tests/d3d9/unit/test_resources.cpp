#include <gtest/gtest.h>
#include <d3d9.h>
#include <cstdint>
#include <cstring>

namespace
{
    IDirect3DDevice9* MakeDevice()
    {
        IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
        D3DPRESENT_PARAMETERS pp = {};
        pp.BackBufferWidth  = 16;
        pp.BackBufferHeight = 16;
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
}

// --- Textures -----------------------------------------------------------

TEST(D3D9Resources, Texture_Create4x4_DefaultMipsAnd4x4Level0)
{
    IDirect3DDevice9* dev = MakeDevice();
    IDirect3DTexture9* tex = nullptr;
    ASSERT_EQ(dev->CreateTexture(4, 4, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr), S_OK);

    // Width=4 -> 4,2,1 => 3 mip levels.
    EXPECT_EQ(tex->GetLevelCount(), 3u);

    D3DSURFACE_DESC desc = {};
    ASSERT_EQ(tex->GetLevelDesc(0, &desc), S_OK);
    EXPECT_EQ(desc.Width,  4u);
    EXPECT_EQ(desc.Height, 4u);
    EXPECT_EQ(desc.Format, D3DFMT_A8R8G8B8);

    ASSERT_EQ(tex->GetLevelDesc(2, &desc), S_OK);
    EXPECT_EQ(desc.Width,  1u);
    EXPECT_EQ(desc.Height, 1u);

    // Out-of-range level rejected.
    EXPECT_EQ(tex->GetLevelDesc(99, &desc), D3DERR_INVALIDCALL);

    tex->Release();
    dev->Release();
}

TEST(D3D9Resources, Texture_LockWriteUnlockRead_Roundtrip)
{
    IDirect3DDevice9* dev = MakeDevice();
    IDirect3DTexture9* tex = nullptr;
    ASSERT_EQ(dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr), S_OK);

    // Write: each pixel's red channel = x, green = y (over BGRA layout → bytes[2]=R, bytes[1]=G).
    {
        D3DLOCKED_RECT lr = {};
        ASSERT_EQ(tex->LockRect(0, &lr, nullptr, 0), S_OK);
        ASSERT_NE(lr.pBits, nullptr);
        ASSERT_GE(lr.Pitch, 16);  // 4 * 4 bytes
        for (UINT y = 0; y < 4; ++y)
        {
            uint8_t* row = static_cast<uint8_t*>(lr.pBits) + y * lr.Pitch;
            for (UINT x = 0; x < 4; ++x)
            {
                row[x * 4 + 0] = 0x00;                         // B
                row[x * 4 + 1] = static_cast<uint8_t>(y);      // G
                row[x * 4 + 2] = static_cast<uint8_t>(x);      // R
                row[x * 4 + 3] = 0xFF;                         // A
            }
        }
        ASSERT_EQ(tex->UnlockRect(0), S_OK);
    }

    // Read back.
    {
        D3DLOCKED_RECT lr = {};
        ASSERT_EQ(tex->LockRect(0, &lr, nullptr, D3DLOCK_READONLY), S_OK);
        for (UINT y = 0; y < 4; ++y)
        {
            const uint8_t* row = static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch;
            for (UINT x = 0; x < 4; ++x)
            {
                EXPECT_EQ(row[x * 4 + 0], 0x00);
                EXPECT_EQ(row[x * 4 + 1], y);
                EXPECT_EQ(row[x * 4 + 2], x);
                EXPECT_EQ(row[x * 4 + 3], 0xFF);
            }
        }
        ASSERT_EQ(tex->UnlockRect(0), S_OK);
    }

    tex->Release();
    dev->Release();
}

TEST(D3D9Resources, Texture_GetSurfaceLevel_AddRefs)
{
    IDirect3DDevice9* dev = MakeDevice();
    IDirect3DTexture9* tex = nullptr;
    ASSERT_EQ(dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr), S_OK);

    IDirect3DSurface9* surf = nullptr;
    ASSERT_EQ(tex->GetSurfaceLevel(0, &surf), S_OK);
    ASSERT_NE(surf, nullptr);

    // Surface outlives texture's own ref — it was AddRef'd on return.
    tex->Release();

    D3DSURFACE_DESC desc = {};
    ASSERT_EQ(surf->GetDesc(&desc), S_OK);
    EXPECT_EQ(desc.Width, 4u);
    surf->Release();
    dev->Release();
}

// --- Vertex buffer ------------------------------------------------------

TEST(D3D9Resources, VertexBuffer_LockWriteUnlockRead_Roundtrip)
{
    IDirect3DDevice9* dev = MakeDevice();
    IDirect3DVertexBuffer9* vb = nullptr;
    ASSERT_EQ(dev->CreateVertexBuffer(256, 0, D3DFVF_XYZ | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &vb, nullptr), S_OK);

    D3DVERTEXBUFFER_DESC desc = {};
    ASSERT_EQ(vb->GetDesc(&desc), S_OK);
    EXPECT_EQ(desc.Size, 256u);
    EXPECT_EQ(desc.FVF,  D3DFVF_XYZ | D3DFVF_DIFFUSE);
    EXPECT_EQ(desc.Type, D3DRTYPE_VERTEXBUFFER);

    void* p = nullptr;
    ASSERT_EQ(vb->Lock(0, 0, &p, 0), S_OK);
    for (int i = 0; i < 256; ++i)
    {
        static_cast<uint8_t*>(p)[i] = static_cast<uint8_t>(i * 7u);
    }
    ASSERT_EQ(vb->Unlock(), S_OK);

    void* q = nullptr;
    ASSERT_EQ(vb->Lock(16, 32, &q, 0), S_OK);
    for (int i = 0; i < 32; ++i)
    {
        EXPECT_EQ(static_cast<uint8_t*>(q)[i], static_cast<uint8_t>((16 + i) * 7u));
    }
    ASSERT_EQ(vb->Unlock(), S_OK);

    // Double-lock fails.
    ASSERT_EQ(vb->Lock(0, 0, &p, 0), S_OK);
    void* dummy = nullptr;
    EXPECT_EQ(vb->Lock(0, 0, &dummy, 0), D3DERR_INVALIDCALL);
    ASSERT_EQ(vb->Unlock(), S_OK);

    // Out-of-range rejected.
    EXPECT_EQ(vb->Lock(200, 100, &dummy, 0), D3DERR_INVALIDCALL);

    vb->Release();
    dev->Release();
}

// --- Index buffer -------------------------------------------------------

TEST(D3D9Resources, IndexBuffer_LockWriteUnlockRead_Roundtrip)
{
    IDirect3DDevice9* dev = MakeDevice();
    IDirect3DIndexBuffer9* ib = nullptr;
    ASSERT_EQ(dev->CreateIndexBuffer(12 * sizeof(uint16_t), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib, nullptr), S_OK);

    D3DINDEXBUFFER_DESC desc = {};
    ASSERT_EQ(ib->GetDesc(&desc), S_OK);
    EXPECT_EQ(desc.Format, D3DFMT_INDEX16);
    EXPECT_EQ(desc.Size,   12u * sizeof(uint16_t));
    EXPECT_EQ(desc.Type,   D3DRTYPE_INDEXBUFFER);

    uint16_t* idx = nullptr;
    ASSERT_EQ(ib->Lock(0, 0, reinterpret_cast<void**>(&idx), 0), S_OK);
    for (int i = 0; i < 12; ++i)
    {
        idx[i] = static_cast<uint16_t>(i * 3);
    }
    ASSERT_EQ(ib->Unlock(), S_OK);

    uint16_t* readIdx = nullptr;
    ASSERT_EQ(ib->Lock(0, 0, reinterpret_cast<void**>(&readIdx), 0), S_OK);
    for (int i = 0; i < 12; ++i)
    {
        EXPECT_EQ(readIdx[i], i * 3);
    }
    ASSERT_EQ(ib->Unlock(), S_OK);

    ib->Release();
    dev->Release();
}

// --- Offscreen plain surface --------------------------------------------

TEST(D3D9Resources, OffscreenPlainSurface_CreateAndLock)
{
    IDirect3DDevice9* dev = MakeDevice();
    IDirect3DSurface9* surf = nullptr;
    ASSERT_EQ(dev->CreateOffscreenPlainSurface(8, 8, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf, nullptr), S_OK);

    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(surf->LockRect(&lr, nullptr, 0), S_OK);
    std::memset(lr.pBits, 0x7F, static_cast<size_t>(lr.Pitch) * 8);
    ASSERT_EQ(surf->UnlockRect(), S_OK);

    ASSERT_EQ(surf->LockRect(&lr, nullptr, D3DLOCK_READONLY), S_OK);
    const uint8_t* row = static_cast<const uint8_t*>(lr.pBits);
    EXPECT_EQ(row[0], 0x7F);
    EXPECT_EQ(row[31], 0x7F);
    ASSERT_EQ(surf->UnlockRect(), S_OK);

    surf->Release();
    dev->Release();
}

// CreateQuery EVENT: trivially completes; GetData returns S_OK immediately after Issue(END).
TEST(D3D9Resources, CreateQuery_Event_ImmediatelyDone)
{
    IDirect3DDevice9* dev = MakeDevice();

    IDirect3DQuery9* q = nullptr;
    ASSERT_EQ(dev->CreateQuery(D3DQUERYTYPE_EVENT, &q), S_OK);
    ASSERT_NE(q, nullptr);
    EXPECT_EQ(q->GetType(), D3DQUERYTYPE_EVENT);
    EXPECT_EQ(q->GetDataSize(), sizeof(BOOL));

    // Before Issue(END), GetData should return S_FALSE.
    BOOL done = FALSE;
    EXPECT_EQ(q->GetData(&done, sizeof(BOOL), 0), S_FALSE);

    // After Issue(END), synchronous SW renderer is always done.
    ASSERT_EQ(q->Issue(D3DISSUE_END), S_OK);
    ASSERT_EQ(q->GetData(&done, sizeof(BOOL), 0), S_OK);
    EXPECT_TRUE(done);

    q->Release();
    dev->Release();
}

// CreateQuery OCCLUSION: returns 0 samples after Issue(END).
TEST(D3D9Resources, CreateQuery_Occlusion_ReturnsZero)
{
    IDirect3DDevice9* dev = MakeDevice();

    IDirect3DQuery9* q = nullptr;
    ASSERT_EQ(dev->CreateQuery(D3DQUERYTYPE_OCCLUSION, &q), S_OK);
    ASSERT_NE(q, nullptr);

    ASSERT_EQ(q->Issue(D3DISSUE_BEGIN), S_OK);
    ASSERT_EQ(q->Issue(D3DISSUE_END), S_OK);

    DWORD samples = 0xDEAD;
    ASSERT_EQ(q->GetData(&samples, sizeof(DWORD), 0), S_OK);
    EXPECT_EQ(samples, 0u);

    q->Release();
    dev->Release();
}

// IDirect3DCubeTexture9: create, lock all six faces, check desc and roundtrip.
TEST(D3D9Resources, CubeTexture_CreateAndLockFaces)
{
    IDirect3DDevice9* dev = MakeDevice();

    IDirect3DCubeTexture9* cube = nullptr;
    ASSERT_EQ(dev->CreateCubeTexture(8, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &cube, nullptr), S_OK);
    ASSERT_NE(cube, nullptr);

    EXPECT_EQ(cube->GetLevelCount(), 1u);

    D3DSURFACE_DESC desc = {};
    ASSERT_EQ(cube->GetLevelDesc(0, &desc), S_OK);
    EXPECT_EQ(desc.Width, 8u);
    EXPECT_EQ(desc.Height, 8u);
    EXPECT_EQ(desc.Format, D3DFMT_A8R8G8B8);

    // Lock each face and write a distinct value; verify readback.
    for (UINT f = 0; f < 6; ++f)
    {
        D3DLOCKED_RECT lr = {};
        auto face = static_cast<D3DCUBEMAP_FACES>(f);
        ASSERT_EQ(cube->LockRect(face, 0, &lr, nullptr, 0), S_OK);
        uint8_t* row = static_cast<uint8_t*>(lr.pBits);
        row[0] = static_cast<uint8_t>(f * 40);  // distinct value per face
        ASSERT_EQ(cube->UnlockRect(face, 0), S_OK);
    }
    for (UINT f = 0; f < 6; ++f)
    {
        D3DLOCKED_RECT lr = {};
        auto face = static_cast<D3DCUBEMAP_FACES>(f);
        ASSERT_EQ(cube->LockRect(face, 0, &lr, nullptr, 0), S_OK);
        uint8_t val = static_cast<const uint8_t*>(lr.pBits)[0];
        EXPECT_EQ(val, static_cast<uint8_t>(f * 40)) << "Face " << f << " readback mismatch";
        ASSERT_EQ(cube->UnlockRect(face, 0), S_OK);
    }

    // GetCubeMapSurface roundtrip
    IDirect3DSurface9* surf = nullptr;
    ASSERT_EQ(cube->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_X, 0, &surf), S_OK);
    ASSERT_NE(surf, nullptr);
    surf->Release();

    cube->Release();
    dev->Release();
}

// --- Private data -----------------------------------------------------------

TEST(D3D9Resources, PrivateData_SetGetFreeOnTexture)
{
    IDirect3DDevice9* dev = MakeDevice();
    IDirect3DTexture9* tex = nullptr;
    ASSERT_EQ(dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, nullptr), S_OK);

    GUID guid = { 0x12345678, 0x1234, 0x1234, {1,2,3,4,5,6,7,8} };
    const uint32_t value = 0xDEADBEEF;

    EXPECT_EQ(tex->SetPrivateData(guid, &value, sizeof(value), 0), S_OK);

    uint32_t readback = 0;
    DWORD size = sizeof(readback);
    EXPECT_EQ(tex->GetPrivateData(guid, &readback, &size), S_OK);
    EXPECT_EQ(readback, value);
    EXPECT_EQ(size, static_cast<DWORD>(sizeof(value)));

    EXPECT_EQ(tex->FreePrivateData(guid), S_OK);
    EXPECT_EQ(tex->FreePrivateData(guid), D3DERR_NOTFOUND);

    tex->Release();
    dev->Release();
}

TEST(D3D9Resources, UpdateTexture_CopiesPixels)
{
    IDirect3DDevice9* dev = MakeDevice();

    IDirect3DTexture9* src = nullptr;
    IDirect3DTexture9* dst = nullptr;
    ASSERT_EQ(dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &src, nullptr), S_OK);
    ASSERT_EQ(dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,   &dst, nullptr), S_OK);

    D3DLOCKED_RECT lr = {};
    ASSERT_EQ(src->LockRect(0, &lr, nullptr, 0), S_OK);
    auto* row = static_cast<uint32_t*>(lr.pBits);
    for (int i = 0; i < 16; ++i) { row[i] = 0xAABBCCDD; }
    ASSERT_EQ(src->UnlockRect(0), S_OK);

    EXPECT_EQ(dev->UpdateTexture(src, dst), S_OK);

    ASSERT_EQ(dst->LockRect(0, &lr, nullptr, 0), S_OK);
    EXPECT_EQ(static_cast<const uint32_t*>(lr.pBits)[7], 0xAABBCCDD);
    ASSERT_EQ(dst->UnlockRect(0), S_OK);

    src->Release();
    dst->Release();
    dev->Release();
}

TEST(D3D9Resources, VolumeTexture_CreateLockUnlock)
{
    IDirect3DDevice9* dev = MakeDevice();

    IDirect3DVolumeTexture9* vol = nullptr;
    ASSERT_EQ(dev->CreateVolumeTexture(4, 4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &vol, nullptr), S_OK);
    ASSERT_NE(vol, nullptr);
    EXPECT_EQ(vol->GetLevelCount(), 1u);

    D3DVOLUME_DESC desc = {};
    ASSERT_EQ(vol->GetLevelDesc(0, &desc), S_OK);
    EXPECT_EQ(desc.Width,  4u);
    EXPECT_EQ(desc.Height, 4u);
    EXPECT_EQ(desc.Depth,  4u);
    EXPECT_EQ(desc.Format, D3DFMT_A8R8G8B8);

    D3DLOCKED_BOX lb = {};
    ASSERT_EQ(vol->LockBox(0, &lb, nullptr, 0), S_OK);
    ASSERT_NE(lb.pBits, nullptr);
    EXPECT_EQ(lb.RowPitch,   4 * 4);   // 4 pixels × 4 bytes
    EXPECT_EQ(lb.SlicePitch, 4 * 4 * 4); // rowPitch × height
    auto* px = static_cast<uint32_t*>(lb.pBits);
    for (int i = 0; i < 4 * 4 * 4; ++i) { px[i] = 0xDEADBEEFu; }
    ASSERT_EQ(vol->UnlockBox(0), S_OK);

    // Re-lock and verify
    ASSERT_EQ(vol->LockBox(0, &lb, nullptr, 0), S_OK);
    EXPECT_EQ(static_cast<uint32_t*>(lb.pBits)[0], 0xDEADBEEFu);
    ASSERT_EQ(vol->UnlockBox(0), S_OK);

    // GetVolumeLevel
    IDirect3DVolume9* level = nullptr;
    ASSERT_EQ(vol->GetVolumeLevel(0, &level), S_OK);
    ASSERT_NE(level, nullptr);
    level->Release();

    vol->Release();
    dev->Release();
}

TEST(D3D9Resources, VolumeTexture_MipLevels)
{
    IDirect3DDevice9* dev = MakeDevice();
    IDirect3DVolumeTexture9* vol = nullptr;
    // levels=0 → auto-generate mips: 8×8×8 → 3 mips (8,4,2,1)
    ASSERT_EQ(dev->CreateVolumeTexture(8, 8, 8, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &vol, nullptr), S_OK);
    EXPECT_EQ(vol->GetLevelCount(), 4u);

    D3DVOLUME_DESC d0{}, d1{};
    vol->GetLevelDesc(0, &d0);
    vol->GetLevelDesc(1, &d1);
    EXPECT_EQ(d0.Width, 8u); EXPECT_EQ(d0.Depth, 8u);
    EXPECT_EQ(d1.Width, 4u); EXPECT_EQ(d1.Depth, 4u);

    vol->Release();
    dev->Release();
}
