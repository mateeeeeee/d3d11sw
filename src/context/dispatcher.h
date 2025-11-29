#pragma once
#include "shaders/shader_abi.h"
#include "shaders/dxbc_parser.h"
#include <memory>

namespace d3d11sw {

class GroupThreadPool;

class D3D11SW_API SWDispatcher
{
public:
    SWDispatcher();
    ~SWDispatcher();

    void DispatchCS(Uint32 groupCountX, Uint32 groupCountY, Uint32 groupCountZ,
                    SW_CSFn fn, SW_Resources& res,
                    const D3D11SW_ParsedShader& shader);

private:
    std::unique_ptr<GroupThreadPool> _pool;
};

}
