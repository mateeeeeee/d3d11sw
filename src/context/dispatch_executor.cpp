#include "context/dispatch_executor.h"
#include <barrier>
#include <cstdlib>
#include <thread>
#include <vector>

namespace d3d11sw {

static void barrierFn(void* ctx)
{
    static_cast<std::barrier<>*>(ctx)->arrive_and_wait();
}

void SWDispatchExecutor::DispatchCS(
    UINT groupCountX, UINT groupCountY, UINT groupCountZ,
    SW_CSFn fn, SW_Resources& res,
    const D3D11SW_ParsedShader& shader)
{
    UINT threadGroupX = shader.threadGroupX > 0 ? shader.threadGroupX : 1;
    UINT threadGroupY = shader.threadGroupY > 0 ? shader.threadGroupY : 1;
    UINT threadGroupZ = shader.threadGroupZ > 0 ? shader.threadGroupZ : 1;
    UINT numThreads   = threadGroupX * threadGroupY * threadGroupZ;

    for (UINT gz = 0; gz < groupCountZ; ++gz)
    {
        for (UINT gy = 0; gy < groupCountY; ++gy)
        {
            for (UINT gx = 0; gx < groupCountX; ++gx)
            {
                SW_TGSM tgsm[SW_MAX_TGSM] = {};
                for (const auto& decl : shader.tgsm)
                {
                    if (decl.slot < SW_MAX_TGSM && decl.size > 0)
                    {
                        tgsm[decl.slot].data   = std::calloc(1, decl.size);
                        tgsm[decl.slot].size   = decl.size;
                        tgsm[decl.slot].stride = decl.stride;
                    }
                }

                std::barrier barrier(numThreads);
                std::vector<std::thread> threads;
                threads.reserve(numThreads);
                D3D11SW_TODO("Use thread pool");
                for (UINT tz = 0; tz < threadGroupZ; ++tz)
                {
                    for (UINT ty = 0; ty < threadGroupY; ++ty)
                    {
                        for (UINT tx = 0; tx < threadGroupX; ++tx)
                        {
                            UINT threadIdx = tz * threadGroupX * threadGroupY
                                           + ty * threadGroupX + tx;

                            SW_CSInput input{};
                            input.groupID          = { gx, gy, gz };
                            input.groupThreadID    = { tx, ty, tz };
                            input.dispatchThreadID = { gx * threadGroupX + tx,
                                                       gy * threadGroupY + ty,
                                                       gz * threadGroupZ + tz };
                            input.groupIndex       = threadIdx;
                            threads.emplace_back([=, &res, &tgsm, &barrier]() 
                            {
                                fn(&input, &res, tgsm, barrierFn, &barrier);
                            });
                        }
                    }
                }

                for (auto& t : threads) 
                { 
                    t.join();
                }

                for (UINT i = 0; i < SW_MAX_TGSM; ++i)
                {
                    std::free(tgsm[i].data);
                }
            }
        }
    }
}

}
