#include <atomic>
#include <barrier>
#include <cstdlib>
#include <cstring>
#include <latch>
#include <semaphore>
#include <thread>
#include <vector>
#include "core/rasterizer/dispatcher.h"
#include "core/common/log.h"
#include "core/shaders/shader_abi.h"
#include "core/shaders/parsed_shader.h"

namespace d3dsw {

struct GroupWork
{
    SW_CSInput    input{};
    SW_CSFn       fn{};
    SW_Resources* res{};
    SW_TGSM*      tgsm{};
};

class GroupThreadPool
{
public:
    explicit GroupThreadPool(Uint32 n) : _n(n), _slots(n), _barrier(n)
    {
        for (Uint32 i = 0; i < n; ++i)
        {
            _threads.emplace_back([this, i]() { Run(i); });
        }
    }

    ~GroupThreadPool()
    {
        _stop.store(true, std::memory_order_release);
        for (auto& s : _slots)
        {
            s.start.release();
        }
        for (auto& t : _threads)
        {
            t.join();
        }
    }

    Uint32 Size() const { return _n; }

    void DispatchGroup(std::vector<GroupWork>& work)
    {
        std::latch done(_n);
        _done = &done;
        for (Uint32 i = 0; i < _n; ++i)
        {
            _slots[i].work = work[i];
            _slots[i].start.release();
        }
        done.wait();
    }

private:
    struct Slot
    {
        GroupWork             work{};
        std::binary_semaphore start{0};
    };

    Uint32                     _n;
    std::vector<Slot>          _slots;
    std::vector<std::thread>   _threads;
    std::barrier<>             _barrier;
    std::latch*                _done = nullptr;
    std::atomic<Bool>          _stop = false;

private:
    static void BarrierCb(void* ctx)
    {
        static_cast<std::barrier<>*>(ctx)->arrive_and_wait();
    }

    void Run(Uint32 i)
    {
        while (true)
        {
            _slots[i].start.acquire();
            if (_stop.load(std::memory_order_acquire)) 
            { 
                return; 
            }

            GroupWork& w = _slots[i].work;
            w.fn(&w.input, w.res, w.tgsm, BarrierCb, &_barrier);
            _done->count_down();
        }
    }
};

class BatchThreadPool
{
public:
    explicit BatchThreadPool(Uint32 n) : _n(n)
    {
        for (Uint32 i = 0; i < n; ++i)
        {
            _threads.emplace_back([this]() { Run(); });
        }
    }

    ~BatchThreadPool()
    {
        _stop.store(true, std::memory_order_release);
        _ready.release(_n);
        for (auto& t : _threads)
        {
            t.join();
        }
    }

    Uint32 Size() const { return _n; }

    void DispatchBatch(SW_CSInput* inputs, Uint32 count,
                       SW_CSFn fn, SW_Resources* res)
    {
        _inputs = inputs;
        _count  = count;
        _fn     = fn;
        _res    = res;
        _next.store(0, std::memory_order_relaxed);
        std::latch done(_n);
        _done = &done;
        _ready.release(_n);
        done.wait();
    }

private:
    Uint32                     _n;
    std::vector<std::thread>   _threads;
    std::counting_semaphore<>  _ready{0};
    std::atomic<Bool>          _stop = false;
    std::latch*                _done = nullptr;

    SW_CSInput*                _inputs = nullptr;
    Uint32                     _count = 0;
    SW_CSFn                    _fn = nullptr;
    SW_Resources*              _res = nullptr;
    std::atomic<Uint32>        _next = 0;

private:
    static void NoBarrier(void*) {}

    void Run()
    {
        while (true)
        {
            _ready.acquire();
            if (_stop.load(std::memory_order_acquire)) 
            { 
                return; 
            }

            while (true)
            {
                Uint32 idx = _next.fetch_add(1, std::memory_order_relaxed);
                if (idx >= _count) 
                { 
                    break; 
                }

                SW_TGSM tgsm[SW_MAX_TGSM] = {};
                _fn(&_inputs[idx], _res, tgsm, NoBarrier, nullptr);
            }
            _done->count_down();
        }
    }
};

SWDispatcher::SWDispatcher()  = default;
SWDispatcher::~SWDispatcher() = default;

void SWDispatcher::Dispatch(
    Uint32 groupCountX, Uint32 groupCountY, Uint32 groupCountZ,
    SWPipelineState& state)
{
    if (!state.cs.fn)
    {
        D3DSW_ERROR("Dispatch: no compute shader bound");
        return;
    }

    SW_CSFn fn = reinterpret_cast<SW_CSFn>(state.cs.fn);
    const D3DSW_ParsedShader& shader = *state.cs.reflection;
    SW_Resources& res = state.csRes;
    Uint32 threadGroupX = shader.csThreadGroupX > 0 ? shader.csThreadGroupX : 1;
    Uint32 threadGroupY = shader.csThreadGroupY > 0 ? shader.csThreadGroupY : 1;
    Uint32 threadGroupZ = shader.csThreadGroupZ > 0 ? shader.csThreadGroupZ : 1;
    const Uint32 numThreads   = threadGroupX * threadGroupY * threadGroupZ;
    Uint32 totalGroups = groupCountX * groupCountY * groupCountZ;
    state.stats->csInvocations += static_cast<Uint64>(totalGroups) * numThreads;
    const Bool useBarriers = !shader.csTgsm.empty();
    if (!useBarriers)
    {
        Uint32 totalWork   = totalGroups * numThreads;

        std::vector<SW_CSInput> inputs(totalWork);
        Uint32 idx = 0;
        for (Uint32 gz = 0; gz < groupCountZ; ++gz)
        {
            for (Uint32 gy = 0; gy < groupCountY; ++gy)
            {
                for (Uint32 gx = 0; gx < groupCountX; ++gx)
                {
                    for (Uint32 tz = 0; tz < threadGroupZ; ++tz)
                    {
                        for (Uint32 ty = 0; ty < threadGroupY; ++ty)
                        {
                            for (Uint32 tx = 0; tx < threadGroupX; ++tx)
                            {
                                SW_CSInput& in = inputs[idx++];
                                in.groupID          = { gx, gy, gz };
                                in.groupThreadID    = { tx, ty, tz };
                                in.dispatchThreadID = { gx * threadGroupX + tx,
                                                        gy * threadGroupY + ty,
                                                        gz * threadGroupZ + tz };
                                in.groupIndex       = tz * threadGroupX * threadGroupY
                                                    + ty * threadGroupX + tx;
                            }
                        }
                    }
                }
            }
        }

        Uint32 hwThreads = std::thread::hardware_concurrency();
        if (hwThreads == 0) 
        { 
            hwThreads = 4; 
        }

        if (!_batchPool || _batchPool->Size() != hwThreads)
        {
            _batchPool = std::make_unique<BatchThreadPool>(hwThreads);
        }

        _batchPool->DispatchBatch(inputs.data(), totalWork, fn, &res);
        return;
    }

    if (!_pool || _pool->Size() != numThreads)
    {
        _pool = std::make_unique<GroupThreadPool>(numThreads);
    }

    Uint32 tgsmTotalSize = 0;
    for (const D3DSW_TGSMDecl& decl : shader.csTgsm)
    {
        if (decl.slot < SW_MAX_TGSM && decl.size > 0)
        {
            tgsmTotalSize += decl.size;
        }
    }

    void* tgsmBuf = tgsmTotalSize > 0 ? std::malloc(tgsmTotalSize) : nullptr;
    std::vector<GroupWork> work(numThreads);
    for (Uint32 i = 0; i < numThreads; ++i)
    {
        work[i].fn  = fn;
        work[i].res = &res;
    }

    Uint32 tz = 0, ty = 0, tx = 0;
    for (Uint32 i = 0; i < numThreads; ++i)
    {
        work[i].input.groupThreadID = { tx, ty, tz };
        work[i].input.groupIndex    = i;
        if (++tx == threadGroupX)
        {
            tx = 0;
            if (++ty == threadGroupY)
            {
                ty = 0;
                ++tz;
            }
        }
    }

    for (Uint32 gz = 0; gz < groupCountZ; ++gz)
    {
        for (Uint32 gy = 0; gy < groupCountY; ++gy)
        {
            for (Uint32 gx = 0; gx < groupCountX; ++gx)
            {
                SW_TGSM tgsm[SW_MAX_TGSM] = {};
                Uint32 tgsmOffset = 0;
                for (const D3DSW_TGSMDecl& decl : shader.csTgsm)
                {
                    if (decl.slot < SW_MAX_TGSM && decl.size > 0)
                    {
                        tgsm[decl.slot].data   = static_cast<Uint8*>(tgsmBuf) + tgsmOffset;
                        tgsm[decl.slot].size   = decl.size;
                        tgsm[decl.slot].stride = decl.stride;
                        tgsmOffset += decl.size;
                    }
                }

                if (tgsmBuf)
                {
                    std::memset(tgsmBuf, 0, tgsmTotalSize);
                }

                for (Uint32 i = 0; i < numThreads; ++i)
                {
                    work[i].tgsm = tgsm;
                    work[i].input.groupID = { gx, gy, gz };
                    work[i].input.dispatchThreadID = {
                        gx * threadGroupX + work[i].input.groupThreadID.x,
                        gy * threadGroupY + work[i].input.groupThreadID.y,
                        gz * threadGroupZ + work[i].input.groupThreadID.z
                    };
                }

                _pool->DispatchGroup(work);
            }
        }
    }

    std::free(tgsmBuf);
}

} 
