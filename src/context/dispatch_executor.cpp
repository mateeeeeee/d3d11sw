#include "context/dispatch_executor.h"

namespace d3d11sw {

void SingleThreadedDispatchExecutor::DispatchCS(
    UINT groupCountX, UINT groupCountY, UINT groupCountZ,
    UINT threadGroupX, UINT threadGroupY, UINT threadGroupZ,
    SW_CSFn fn, const SW_Resources& res)
{
    for (UINT gz = 0; gz < groupCountZ; ++gz)
    {
        for (UINT gy = 0; gy < groupCountY; ++gy)
        {
            for (UINT gx = 0; gx < groupCountX; ++gx)
            {
                for (UINT tz = 0; tz < threadGroupZ; ++tz)
                {
                    for (UINT ty = 0; ty < threadGroupY; ++ty)
                    {
                        for (UINT tx = 0; tx < threadGroupX; ++tx)
                        {
                            SW_CSInput input{};
                            input.groupID          = { gx, gy, gz };
                            input.groupThreadID    = { tx, ty, tz };
                            input.dispatchThreadID = { gx * threadGroupX + tx,
                                                       gy * threadGroupY + ty,
                                                       gz * threadGroupZ + tz };
                            fn(&input, &res);
                        }
                    }
                }
            }
        }
    }
}

}
