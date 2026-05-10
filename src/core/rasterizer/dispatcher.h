#pragma once
#include "core/pipeline/sw_pipeline_state.h"
#include <memory>

namespace d3dsw {

class GroupThreadPool;
class BatchThreadPool;

class D3DSW_API SWDispatcher
{
public:
    SWDispatcher();
    ~SWDispatcher();

    void Dispatch(Uint32 groupCountX, Uint32 groupCountY, Uint32 groupCountZ,
                  SWPipelineState& state);

private:
    std::unique_ptr<GroupThreadPool> _pool;
    std::unique_ptr<BatchThreadPool> _batchPool;
};

}
