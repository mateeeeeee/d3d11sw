#pragma once
#include "context/pipeline_state.h"
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

    void Dispatch(Uint32 groupCountX, Uint32 groupCountY, Uint32 groupCountZ,
                  D3D11SW_PIPELINE_STATE& state);

private:
    std::unique_ptr<GroupThreadPool> _pool;

private:
    void BuildResources(SW_Resources& res, D3D11SW_PIPELINE_STATE& state);
};

}
