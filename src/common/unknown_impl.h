#pragma once
#include "common.h"

namespace d3d11sw {

class UnknownBase
{
protected:
    ULONG AddRefImpl()
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    ULONG ReleaseImpl()
    {
        ULONG count = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (count == 0)
        {
            delete this;
        }
        return count;
    }

    virtual ~UnknownBase() = default;


protected:
    std::atomic<ULONG> _refCount{1};
};
}
