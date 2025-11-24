#pragma once
#include "shaders/shader_abi.h"

namespace d3d11sw {

class ISWDispatchExecutor
{
public:
    virtual ~ISWDispatchExecutor() = default;

    virtual void DispatchCS(UINT groupCountX, UINT groupCountY, UINT groupCountZ,
                            UINT threadGroupX, UINT threadGroupY, UINT threadGroupZ,
                            SW_CSFn fn, const SW_Resources& res) = 0;
};

class SingleThreadedDispatchExecutor final : public ISWDispatchExecutor
{
public:
    void DispatchCS(UINT groupCountX, UINT groupCountY, UINT groupCountZ,
                    UINT threadGroupX, UINT threadGroupY, UINT threadGroupZ,
                    SW_CSFn fn, const SW_Resources& res) override;
};

}
