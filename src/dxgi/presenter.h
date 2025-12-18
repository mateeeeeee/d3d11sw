#pragma once
#include <memory>
#include <cstdint>

namespace d3d11sw {

class ISwapChainPresenter
{
public:
    virtual ~ISwapChainPresenter() = default;
    virtual void Present(const void* bgraData, uint32_t width, uint32_t height, uint32_t rowPitch) = 0;
};

std::unique_ptr<ISwapChainPresenter> CreatePresenter(void* hwnd);

}
