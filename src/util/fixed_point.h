#pragma once
#include "common/macros.h"

namespace d3d11sw {

template<typename StorageT, Usize FracBits>
class FixedPoint
{
    static constexpr StorageT Scale = StorageT(1) << FracBits;
public:
    FixedPoint() = default;
    constexpr explicit FixedPoint(StorageT v) : _raw(v) {}

    static FixedPoint FromFloat(Float v) { return FixedPoint(static_cast<StorageT>(v * Scale + 0.5f)); }
    Float ToFloat() const { return static_cast<Float>(_raw) / static_cast<Float>(Scale); }
    explicit operator Float() const { return static_cast<Float>(_raw); }

    FixedPoint  operator+ (FixedPoint r) const { return FixedPoint(_raw + r._raw); }
    FixedPoint  operator- (FixedPoint r) const { return FixedPoint(_raw - r._raw); }
    FixedPoint  operator- ()             const { return FixedPoint(-_raw); }
    FixedPoint  operator* (FixedPoint r) const { return FixedPoint(_raw * r._raw); }
    FixedPoint  operator* (StorageT r)   const { return FixedPoint(_raw * r); }
    FixedPoint& operator+=(FixedPoint r) { _raw += r._raw; return *this; }
    FixedPoint& operator-=(FixedPoint r) { _raw -= r._raw; return *this; }

    Bool operator==(FixedPoint r) const { return _raw == r._raw; }
    Bool operator!=(FixedPoint r) const { return _raw != r._raw; }
    Bool operator>=(FixedPoint r) const { return _raw >= r._raw; }
    Bool operator> (FixedPoint r) const { return _raw >  r._raw; }
    Bool operator<=(FixedPoint r) const { return _raw <= r._raw; }
    Bool operator< (FixedPoint r) const { return _raw <  r._raw; }
    Bool operator==(StorageT r)   const { return _raw == r; }
    Bool operator!=(StorageT r)   const { return _raw != r; }
    Bool operator>=(StorageT r)   const { return _raw >= r; }
    Bool operator> (StorageT r)   const { return _raw >  r; }
    Bool operator<=(StorageT r)   const { return _raw <= r; }
    Bool operator< (StorageT r)   const { return _raw <  r; }

private:
    StorageT _raw;
};

using Fixed28_4 = FixedPoint<Int64, 4>;

}
