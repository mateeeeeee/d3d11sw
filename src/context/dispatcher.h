#pragma once
#include <memory>

namespace d3d11sw {

class GroupThreadPool;
class BatchThreadPool;
struct SW_Resources;
struct D3D11SW_PIPELINE_STATE;

class D3D11SW_API SWDispatcher
{
public:
    SWDispatcher();
    ~SWDispatcher();

    void Dispatch(Uint32 groupCountX, Uint32 groupCountY, Uint32 groupCountZ,
                  D3D11SW_PIPELINE_STATE& state);

private:
    std::unique_ptr<GroupThreadPool> _pool;
    std::unique_ptr<BatchThreadPool> _batchPool;

private:
    void BuildResources(SW_Resources& res, D3D11SW_PIPELINE_STATE& state);
};

}
