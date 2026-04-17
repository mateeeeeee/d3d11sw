#include "common/common.h"
#include "common/log.h"
#include "device/device.h"
#include "context/context.h"
#include "dxgi/swapchain.h"
#include "dxgi/factory.h"
#include "dxgi/adapter.h"

using namespace d3d11sw;

static HRESULT CreateDeviceAndContext(UINT Flags, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext)
{
    Bool debug = (Flags & D3D11_CREATE_DEVICE_DEBUG) != 0;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    if (debug)
    {
        auto* dev = new D3D11DebugDeviceSW();
        auto* ctx = new D3D11DebugDeviceContextSW(dev);
        dev->SetImmediateContext(ctx);
        device = dev;
        context = ctx;
        D3D11SW_INFO("D3D11CreateDevice: debug layer enabled");
    }
    else
    {
        auto* dev = new D3D11DeviceSW();
        auto* ctx = new D3D11DeviceContextSW(dev);
        dev->SetImmediateContext(ctx);
        device = dev;
        context = ctx;
    }

    if (pFeatureLevel)
    {
        *pFeatureLevel = D3D_FEATURE_LEVEL_11_1;
    }

    if (ppDevice)
    {
        *ppDevice = device;
    }
    else
    {
        device->Release();
    }

    if (ppImmediateContext)
    {
        *ppImmediateContext = context;
    }
    else
    {
        context->Release();
    }

    return S_OK;
}

extern "C"
{

HRESULT WINAPI D3D11CreateDevice(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    const D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext)
{
    D3D11SW_INFO("D3D11CreateDevice called");

    if (!ppDevice && !ppImmediateContext)
    {
        return S_FALSE;
    }

    return CreateDeviceAndContext(Flags, ppDevice, pFeatureLevel, ppImmediateContext);
}

HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
    IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    const D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    IDXGISwapChain** ppSwapChain,
    ID3D11Device** ppDevice,
    D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext)
{
    D3D11SW_INFO("D3D11CreateDeviceAndSwapChain called");

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    HRESULT hr = CreateDeviceAndContext(Flags, &device, pFeatureLevel, &context);
    if (FAILED(hr))
    {
        return hr;
    }

    if (ppSwapChain)
    {
        if (!pSwapChainDesc)
        {
            device->Release();
            context->Release();
            return DXGI_ERROR_INVALID_CALL;
        }
        DXGISwapChainSW* swapChain = new DXGISwapChainSW(device, *pSwapChainDesc);
        *ppSwapChain = swapChain;
    }

    if (ppDevice)
    {
        *ppDevice = device;
    }
    else
    {
        device->Release();
    }

    if (ppImmediateContext)
    {
        *ppImmediateContext = context;
    }
    else
    {
        context->Release();
    }
    return S_OK;
}

}
