#pragma once
#include <atomic>
#include <latch>
#include <semaphore>
#include <thread>
#include <vector>

namespace d3dsw {

class TileThreadPool
{
public:
    explicit TileThreadPool(Uint32 n) : _n(n)
    {
        for (Uint32 i = 0; i < n; ++i)
        {
            _threads.emplace_back([this]() { Run(); });
        }
    }

    ~TileThreadPool()
    {
        _stop.store(true, std::memory_order_release);
        _ready.release(_n);
        for (auto& t : _threads)
        {
            t.join();
        }
    }

    Uint32 Size() const { return _n; }

    using TileFn = void(*)(void* ctx, Uint32 tileIdx);
    void Dispatch(Uint32 count, TileFn fn, void* ctx)
    {
        _fn    = fn;
        _ctx   = ctx;
        _count = count;
        _next.store(0, std::memory_order_relaxed);

        std::latch done(_n + 1);
        _done = &done;
        _ready.release(_n);
        while (true)
        {
            Uint32 idx = _next.fetch_add(1, std::memory_order_relaxed);
            if (idx >= _count)
            {
                break;
            }

            _fn(_ctx, idx);
        }
        done.count_down();
        done.wait();
    }

private:
    Uint32                     _n;
    std::vector<std::thread>   _threads;
    std::counting_semaphore<>  _ready{0};
    std::atomic<Bool>          _stop = false;
    std::latch*                _done = nullptr;
    TileFn                     _fn = nullptr;
    void*                      _ctx = nullptr;
    Uint32                     _count = 0;
    std::atomic<Uint32>        _next = 0;

private:
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
                _fn(_ctx, idx);
            }
            _done->count_down();
        }
    }
};

}
