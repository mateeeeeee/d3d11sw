#include "common/common.h"
#include "common/log.h"
#include "device/device.h"
#include "context/context.h"
#include "dxgi/swapchain.h"
#include "dxgi/factory.h"
#include "dxgi/adapter.h"

using namespace d3d11sw;

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

    Direct3D11DeviceSW* device = new Direct3D11DeviceSW();
    Direct3D11DeviceContextSW* context = new Direct3D11DeviceContextSW(device);
    device->SetImmediateContext(context);

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

    Direct3D11DeviceSW* device = new Direct3D11DeviceSW();
    Direct3D11DeviceContextSW* context = new Direct3D11DeviceContextSW(device);
    device->SetImmediateContext(context);

    if (pFeatureLevel) 
    {
        *pFeatureLevel = D3D_FEATURE_LEVEL_11_1;
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
