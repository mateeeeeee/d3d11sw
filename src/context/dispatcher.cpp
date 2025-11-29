#include "context/dispatcher.h"
#include <barrier>
#include <cstdlib>
#include <latch>
#include <semaphore>
#include <thread>
#include <vector>

namespace d3d11sw {

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

    void Dispatch(std::vector<GroupWork>& work)
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
    std::latch*                _done{nullptr};
    std::atomic<Bool>          _stop{false};

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
            if (_stop.load(std::memory_order_acquire)) { return; }
            GroupWork& w = _slots[i].work;
            w.fn(&w.input, w.res, w.tgsm, BarrierCb, &_barrier);
            _done->count_down();
        }
    }
};

SWDispatcher::SWDispatcher()  = default;
SWDispatcher::~SWDispatcher() = default;

void SWDispatcher::DispatchCS(
    Uint32 groupCountX, Uint32 groupCountY, Uint32 groupCountZ,
    SW_CSFn fn, SW_Resources& res,
    const D3D11SW_ParsedShader& shader)
{
    Uint32 threadGroupX = shader.threadGroupX > 0 ? shader.threadGroupX : 1;
    Uint32 threadGroupY = shader.threadGroupY > 0 ? shader.threadGroupY : 1;
    Uint32 threadGroupZ = shader.threadGroupZ > 0 ? shader.threadGroupZ : 1;
    Uint32 numThreads   = threadGroupX * threadGroupY * threadGroupZ;

    if (!_pool || _pool->Size() != numThreads)
    {
        _pool = std::make_unique<GroupThreadPool>(numThreads);
    }

    std::vector<GroupWork> work;
    work.reserve(numThreads);

    for (Uint32 gz = 0; gz < groupCountZ; ++gz)
    {
        for (Uint32 gy = 0; gy < groupCountY; ++gy)
        {
            for (Uint32 gx = 0; gx < groupCountX; ++gx)
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

                work.clear();
                for (Uint32 tz = 0; tz < threadGroupZ; ++tz)
                {
                    for (Uint32 ty = 0; ty < threadGroupY; ++ty)
                    {
                        for (Uint32 tx = 0; tx < threadGroupX; ++tx)
                        {
                            Uint32 threadIdx = tz * threadGroupX * threadGroupY
                                           + ty * threadGroupX + tx;
                            GroupWork gw{};
                            gw.fn  = fn;
                            gw.res = &res;
                            gw.tgsm = tgsm;
                            gw.input.groupID          = { gx, gy, gz };
                            gw.input.groupThreadID    = { tx, ty, tz };
                            gw.input.dispatchThreadID = { gx * threadGroupX + tx,
                                                          gy * threadGroupY + ty,
                                                          gz * threadGroupZ + tz };
                            gw.input.groupIndex       = threadIdx;
                            work.push_back(gw);
                        }
                    }
                }

                _pool->Dispatch(work);

                for (Uint32 i = 0; i < SW_MAX_TGSM; ++i)
                {
                    std::free(tgsm[i].data);
                }
            }
        }
    }
}

} 
