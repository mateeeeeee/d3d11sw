#include "dxgi/output.h"
#include <algorithm>
#include <cstring>

namespace d3d11sw {

HRESULT STDMETHODCALLTYPE DXGIOutputSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(IDXGIOutput1))
    {
        *ppv = static_cast<IDXGIOutput1*>(this);
    }
    else if (riid == __uuidof(IDXGIOutput))
    {
        *ppv = static_cast<IDXGIOutput*>(this);
    }
    else if (riid == __uuidof(IDXGIObject))
    {
        *ppv = static_cast<IDXGIObject*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGIOutputSW::GetDesc(DXGI_OUTPUT_DESC* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }
    memset(pDesc, 0, sizeof(*pDesc));
    wcscpy(pDesc->DeviceName, L"d3d11sw Output");
    pDesc->DesktopCoordinates = {0, 0, 1920, 1080};
    pDesc->AttachedToDesktop = TRUE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGIOutputSW::GetDisplayModeList(DXGI_FORMAT EnumFormat, UINT Flags, UINT* pNumModes, DXGI_MODE_DESC* pDesc)
{
    if (!pNumModes)
    {
        return E_INVALIDARG;
    }

    if (EnumFormat != DXGI_FORMAT_R8G8B8A8_UNORM &&
        EnumFormat != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
    {
        *pNumModes = 0;
        return S_OK;
    }

    const DXGI_MODE_DESC modes[] =
    {
        { 800,  600,  {60, 1}, EnumFormat, DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE, DXGI_MODE_SCALING_UNSPECIFIED},
        {1024,  768,  {60, 1}, EnumFormat, DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE, DXGI_MODE_SCALING_UNSPECIFIED},
        {1280,  720,  {60, 1}, EnumFormat, DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE, DXGI_MODE_SCALING_UNSPECIFIED},
        {1920, 1080,  {60, 1}, EnumFormat, DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE, DXGI_MODE_SCALING_UNSPECIFIED},
    };
    constexpr UINT numModes = sizeof(modes) / sizeof(modes[0]);

    if (!pDesc)
    {
        *pNumModes = numModes;
        return S_OK;
    }

    UINT count = (*pNumModes < numModes) ? *pNumModes : numModes;
    memcpy(pDesc, modes, count * sizeof(DXGI_MODE_DESC));
    *pNumModes = count;
    return (count < numModes) ? DXGI_ERROR_MORE_DATA : S_OK;
}

HRESULT STDMETHODCALLTYPE DXGIOutputSW::FindClosestMatchingMode(const DXGI_MODE_DESC* pModeToMatch, DXGI_MODE_DESC* pClosestMatch, IUnknown* pConcernedDevice)
{
    if (!pModeToMatch || !pClosestMatch)
    {
        return E_INVALIDARG;
    }
    *pClosestMatch = *pModeToMatch;
    if (pClosestMatch->Width == 0)
    {
        pClosestMatch->Width = 1920;
    }
    if (pClosestMatch->Height == 0)
    {
        pClosestMatch->Height = 1080;
    }
    if (pClosestMatch->RefreshRate.Numerator == 0)
    {
        pClosestMatch->RefreshRate = {60, 1};
    }
    if (pClosestMatch->Format == DXGI_FORMAT_UNKNOWN)
    {
        pClosestMatch->Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    return S_OK;
}

}
