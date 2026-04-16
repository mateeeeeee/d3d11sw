#include "dxgi/factory.h"

using namespace d3d11sw;

static HRESULT CreateFactory(REFIID riid, void** ppFactory)
{
    if (!ppFactory)
    {
        return DXGI_ERROR_INVALID_CALL;
    }

    DXGIFactorySW* factory = new DXGIFactorySW();
    HRESULT hr = factory->QueryInterface(riid, ppFactory);
    factory->Release();
    return hr;
}

extern "C"
{

HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** ppFactory)
{
    return CreateFactory(riid, ppFactory);
}

HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** ppFactory)
{
    return CreateFactory(riid, ppFactory);
}

HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, void** ppFactory)
{
    return CreateFactory(riid, ppFactory);
}

}
