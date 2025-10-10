#include <d3d11.h>
#include <cassert>
#include <cstdio>

int main() {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &device,
        &featureLevel,
        &context);

    assert(SUCCEEDED(hr));
    assert(device != nullptr);
    assert(context != nullptr);

    printf("MARS: Device created successfully!\n");
    printf("MARS: Feature level: 0x%x\n", featureLevel);

    ID3D11Device1* device1 = nullptr;
    hr = device->QueryInterface(__uuidof(ID3D11Device1), (void**)&device1);
    if (SUCCEEDED(hr)) {
        printf("MARS: QueryInterface for ID3D11Device1 succeeded\n");
        device1->Release();
    }

    context->Release();
    device->Release();

    printf("MARS: All tests passed!\n");
    return 0;
}
