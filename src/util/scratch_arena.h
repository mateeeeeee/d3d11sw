#pragma once
#include "common/macros.h"

namespace d3d11sw {

class ScratchArena
{
public:
    static constexpr Usize DefaultSize = 128 * 1024;

    ScratchArena() : _savedOffset(Offset())
    {
    }

    ~ScratchArena()
    {
        Offset() = _savedOffset;
    }

    template<typename T>
    T* Alloc(Usize count)
    {
        Usize& off = Offset();
        Usize align = alignof(T);
        Usize aligned = (off + align - 1) & ~(align - 1);
        off = aligned + sizeof(T) * count;
        return reinterpret_cast<T*>(Storage() + aligned);
    }

private:
    Usize _savedOffset;

private:
    static Uint8* Storage()
    {
        thread_local Uint8 buf[DefaultSize];
        return buf;
    }

    static Usize& Offset()
    {
        thread_local Usize off = 0;
        return off;
    }
};

}
