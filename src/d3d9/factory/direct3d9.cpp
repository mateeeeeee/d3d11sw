#include "d3d9/factory/direct3d9.h"
#include "d3d9/device/device.h"
#include "core/common/log.h"
#include <cstring>

namespace d3dsw {

namespace {

struct ModeEntry { UINT width; UINT height; };
static constexpr ModeEntry kModes[] = {
    {  640,  480 }, {  800,  600 }, { 1024,  768 },
    { 1280,  720 }, { 1280,  800 }, { 1280,  960 }, { 1280, 1024 },
    { 1360,  768 }, { 1366,  768 },
    { 1440,  900 }, { 1600,  900 }, { 1680, 1050 },
    { 1920, 1080 }, { 1920, 1200 },
    { 2560, 1440 }, { 3840, 2160 },
};
static constexpr UINT kNumModes    = sizeof(kModes) / sizeof(kModes[0]);
static constexpr UINT kRefreshRate = 60;

bool IsFormatSupported(D3DFORMAT fmt)
{
    return fmt == D3DFMT_X8R8G8B8 || fmt == D3DFMT_A8R8G8B8 || fmt == D3DFMT_R5G6B5;
}

}  

D3D9SW::D3D9SW() = default;
D3D9SW::~D3D9SW() = default;

HRESULT STDMETHODCALLTYPE D3D9SW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) { return E_POINTER; }
    if (riid == IID_IUnknown || riid == IID_IDirect3D9Ex)
    {
        *ppv = static_cast<IDirect3D9Ex*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_IDirect3D9)
    {
        *ppv = static_cast<IDirect3D9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE D3D9SW::RegisterSoftwareDevice(void*) { return D3DERR_NOTAVAILABLE; }
UINT    STDMETHODCALLTYPE D3D9SW::GetAdapterCount() { return 1; }
HRESULT STDMETHODCALLTYPE D3D9SW::GetAdapterIdentifier(UINT Adapter, DWORD /*Flags*/, D3DADAPTER_IDENTIFIER9* pIdentifier)
{
    if (Adapter != 0 || !pIdentifier) { return D3DERR_INVALIDCALL; }
    std::memset(pIdentifier, 0, sizeof(*pIdentifier));
    std::strncpy(pIdentifier->Driver,      "d3d9sw.dll",                    MAX_DEVICE_IDENTIFIER_STRING - 1);
    std::strncpy(pIdentifier->Description, "D3D9SW Software Rasterizer",    MAX_DEVICE_IDENTIFIER_STRING - 1);
    std::strncpy(pIdentifier->DeviceName,  "\\\\.\\DISPLAY1",               32 - 1);
    pIdentifier->DriverVersion.QuadPart = 1;
    return S_OK;
}
UINT    STDMETHODCALLTYPE D3D9SW::GetAdapterModeCount(UINT Adapter, D3DFORMAT Format)
{
    if (Adapter != 0 || !IsFormatSupported(Format)) { return 0; }
    return kNumModes;
}
HRESULT STDMETHODCALLTYPE D3D9SW::EnumAdapterModes(UINT Adapter, D3DFORMAT Format,
                                                    UINT Mode, D3DDISPLAYMODE* pMode)
{
    if (Adapter != 0 || !pMode)                    { return D3DERR_INVALIDCALL; }
    if (!IsFormatSupported(Format) || Mode >= kNumModes) { return D3DERR_INVALIDCALL; }
    pMode->Width       = kModes[Mode].width;
    pMode->Height      = kModes[Mode].height;
    pMode->RefreshRate = kRefreshRate;
    pMode->Format      = Format;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9SW::GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode)
{
    if (Adapter != 0 || !pMode) { return D3DERR_INVALIDCALL; }
    pMode->Width       = 1920;
    pMode->Height      = 1080;
    pMode->RefreshRate = 60;
    pMode->Format      = D3DFMT_X8R8G8B8;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D9SW::CheckDeviceType(UINT, D3DDEVTYPE, D3DFORMAT, D3DFORMAT, BOOL) { return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9SW::CheckDeviceFormat(UINT, D3DDEVTYPE, D3DFORMAT, DWORD, D3DRESOURCETYPE, D3DFORMAT) { return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9SW::CheckDeviceMultiSampleType(UINT, D3DDEVTYPE, D3DFORMAT, BOOL, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels) 
{ 
    if (MultiSampleType == D3DMULTISAMPLE_NONE)
    {
        if (pQualityLevels)
        {
            *pQualityLevels = 1;
        }
        return S_OK;
    }
    return D3DERR_NOTAVAILABLE;
}
HRESULT STDMETHODCALLTYPE D3D9SW::CheckDepthStencilMatch(UINT, D3DDEVTYPE, D3DFORMAT, D3DFORMAT, D3DFORMAT) { return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9SW::CheckDeviceFormatConversion(UINT, D3DDEVTYPE, D3DFORMAT, D3DFORMAT) { return S_OK; }
HRESULT STDMETHODCALLTYPE D3D9SW::GetDeviceCaps(UINT Adapter, D3DDEVTYPE, D3DCAPS9* pCaps)
{
    if (Adapter != 0 || !pCaps) { return D3DERR_INVALIDCALL; }
    std::memset(pCaps, 0, sizeof(*pCaps));
    pCaps->DeviceType            = D3DDEVTYPE_HAL;
    pCaps->AdapterOrdinal        = 0;
    pCaps->VertexShaderVersion   = D3DVS_VERSION(3, 0);
    pCaps->PixelShaderVersion    = D3DPS_VERSION(3, 0);
    pCaps->MaxVertexShaderConst  = 256;
    pCaps->MaxStreams             = 16;
    pCaps->MaxStreamStride       = 65536;
    pCaps->MaxTextureWidth       = 4096;
    pCaps->MaxTextureHeight      = 4096;
    pCaps->MaxTextureRepeat      = 8192;
    pCaps->MaxTextureAspectRatio = 4096;
    pCaps->MaxAnisotropy         = 16;
    pCaps->NumSimultaneousRTs    = 4;
    pCaps->MaxPrimitiveCount     = 0x7FFFFFFFu;
    pCaps->MaxVertexIndex        = 0x7FFFFFFFu;
    pCaps->DevCaps               = D3DDEVCAPS_HWTRANSFORMANDLIGHT | D3DDEVCAPS_HWRASTERIZATION;
    pCaps->RasterCaps            = D3DPRASTERCAPS_FOGTABLE;
    pCaps->Caps2                 = D3DCAPS2_DYNAMICTEXTURES;
    pCaps->TextureCaps           = D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_NONPOW2CONDITIONAL |
                                   D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_MIPMAP;
    pCaps->TextureFilterCaps     = D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR |
                                   D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR |
                                   D3DPTFILTERCAPS_MIPFPOINT | D3DPTFILTERCAPS_MIPFLINEAR;
    pCaps->SrcBlendCaps          = 0xFFFF;
    pCaps->DestBlendCaps         = 0xFFFF;
    pCaps->AlphaCmpCaps          = 0xFF;
    pCaps->ZCmpCaps              = 0xFF;
    pCaps->StencilCaps           = 0xFF;
    pCaps->MaxVertexW            = 1e10f;
    pCaps->VertexShaderVersion   = D3DVS_VERSION(3, 0);
    pCaps->PixelShaderVersion    = D3DPS_VERSION(3, 0);
    pCaps->PixelShader1xMaxValue = 8.f;
    return S_OK;
}
HMONITOR STDMETHODCALLTYPE D3D9SW::GetAdapterMonitor(UINT) { return nullptr; }
HRESULT STDMETHODCALLTYPE D3D9SW::CreateDevice(UINT /*Adapter*/, D3DDEVTYPE /*DeviceType*/,
                                               HWND hFocusWindow, DWORD /*BehaviorFlags*/,
                                               D3DPRESENT_PARAMETERS* pPresentationParameters,
                                               IDirect3DDevice9** ppReturnedDeviceInterface)
{
    if (!ppReturnedDeviceInterface || !pPresentationParameters)
    {
        return D3DERR_INVALIDCALL;
    }
    *ppReturnedDeviceInterface = new D3D9DeviceSW(this, *pPresentationParameters, hFocusWindow);
    return S_OK;
}

// ---------------------------------------------------------------------------
// IDirect3D9Ex — all new methods delegate to base or stub
// ---------------------------------------------------------------------------

UINT STDMETHODCALLTYPE D3D9SW::GetAdapterModeCountEx(UINT Adapter, const D3DDISPLAYMODEFILTER* pFilter)
{
    D3DFORMAT fmt = pFilter ? pFilter->Format : D3DFMT_X8R8G8B8;
    return GetAdapterModeCount(Adapter, fmt);
}

HRESULT STDMETHODCALLTYPE D3D9SW::EnumAdapterModesEx(UINT Adapter, const D3DDISPLAYMODEFILTER* pFilter, UINT Mode, D3DDISPLAYMODEEX* pMode)
{
    if (!pMode) { return D3DERR_INVALIDCALL; }
    D3DFORMAT fmt = pFilter ? pFilter->Format : D3DFMT_X8R8G8B8;
    D3DDISPLAYMODE base{};
    HRESULT hr = EnumAdapterModes(Adapter, fmt, Mode, &base);
    if (FAILED(hr)) { return hr; }
    pMode->Size             = sizeof(D3DDISPLAYMODEEX);
    pMode->Width            = base.Width;
    pMode->Height           = base.Height;
    pMode->RefreshRate      = base.RefreshRate;
    pMode->Format           = base.Format;
    pMode->ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9SW::GetAdapterDisplayModeEx(UINT Adapter, D3DDISPLAYMODEEX* pMode, D3DDISPLAYROTATION* pRotation)
{
    if (pRotation) 
    { 
        *pRotation = D3DDISPLAYROTATION_IDENTITY; 
    }

    if (!pMode)    
    { 
        return S_OK; 
    }

    D3DDISPLAYMODE base{};
    HRESULT hr = GetAdapterDisplayMode(Adapter, &base);
    if (FAILED(hr)) 
    { 
        return hr; 
    }
    pMode->Size             = sizeof(D3DDISPLAYMODEEX);
    pMode->Width            = base.Width;
    pMode->Height           = base.Height;
    pMode->RefreshRate      = base.RefreshRate;
    pMode->Format           = base.Format;
    pMode->ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9SW::CreateDeviceEx(UINT, D3DDEVTYPE, HWND hFocusWindow,
                                                  DWORD, D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                  D3DDISPLAYMODEEX*,
                                                  IDirect3DDevice9Ex** ppReturnedDeviceInterface)
{
    if (!ppReturnedDeviceInterface || !pPresentationParameters) 
    { 
        return D3DERR_INVALIDCALL; 
    }

    D3D9DeviceSW* dev = new D3D9DeviceSW(this, *pPresentationParameters, hFocusWindow, true);
    *ppReturnedDeviceInterface = dev;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9SW::GetAdapterLUID(UINT Adapter, LUID* pLUID)
{
    if (!pLUID || Adapter != D3DADAPTER_DEFAULT) { return D3DERR_INVALIDCALL; }
    pLUID->LowPart  = 0xD3D9'5700u;
    pLUID->HighPart = 0;
    return S_OK;
}

}
