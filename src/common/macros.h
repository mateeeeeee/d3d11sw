#pragma once
#include <cassert>

#define _D3D11SW_STRINGIFY_IMPL(a) #a
#define _D3D11SW_CONCAT_IMPL(x, y) x##y

#define D3D11SW_STRINGIFY(a) _D3D11SW_STRINGIFY_IMPL(a)
#define D3D11SW_CONCAT(x, y) _D3D11SW_CONCAT_IMPL(x, y)

#define D3D11SW_TODO(...)
#define D3D11SW_HACK(stmt, msg) stmt

#define D3D11SW_ASSERT(expr)            assert(expr)
#define D3D11SW_ASSERT_MSG(expr, msg)   assert((expr) && (msg))

#if defined(_MSC_VER)
#define D3D11SW_DEBUGBREAK()       __debugbreak()
#define D3D11SW_UNREACHABLE()      __assume(false)
#define D3D11SW_FORCEINLINE        __forceinline
#define D3D11SW_NOINLINE           __declspec(noinline)
#define D3D11SW_DEBUGZONE_BEGIN    __pragma(optimize("", off))
#define D3D11SW_DEBUGZONE_END      __pragma(optimize("", on))

#elif defined(__GNUC__) || defined(__clang__)
#define D3D11SW_DEBUGBREAK()       __builtin_trap()
#define D3D11SW_UNREACHABLE()      __builtin_unreachable()
#define D3D11SW_FORCEINLINE        inline __attribute__((always_inline))
#define D3D11SW_NOINLINE           __attribute__((noinline))
#define D3D11SW_DEBUGZONE_BEGIN    _Pragma("GCC push_options") \
                               _Pragma("GCC optimize(\"O0\")")
#define D3D11SW_DEBUGZONE_END      _Pragma("GCC pop_options")

#else
#define D3D11SW_DEBUGBREAK()       ((void)0)
#define D3D11SW_UNREACHABLE()      ((void)0)
#define D3D11SW_FORCEINLINE        inline
#define D3D11SW_NOINLINE
#define D3D11SW_DEBUGZONE_BEGIN
#define D3D11SW_DEBUGZONE_END
#endif

#define D3D11SW_ALIGN(x, align)    ((x) & ~((align) - 1))
#define D3D11SW_ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))

#define D3D11SW_NODISCARD           [[nodiscard]]
#define D3D11SW_NORETURN            [[noreturn]]
#define D3D11SW_DEPRECATED          [[deprecated]]
#define D3D11SW_MAYBE_UNUSED        [[maybe_unused]]
#define D3D11SW_DEPRECATED_MSG(msg) [[deprecated(msg)]]

#if defined(_WIN32) || defined(_WIN64)
#define D3D11SW_PLATFORM_WINDOWS
#elif defined(__linux__)
#define D3D11SW_PLATFORM_LINUX
#elif defined(__APPLE__)
#define D3D11SW_PLATFORM_MACOS
#endif

#ifdef D3D11SW_PLATFORM_WINDOWS
#  ifdef D3D11SW_BUILDING_DLL
#    define D3D11SW_API __declspec(dllexport)
#  else
#    define D3D11SW_API __declspec(dllimport)
#  endif
#else
#  define D3D11SW_API
#endif

#define D3D11SW_NONCOPYABLE(Class)                   \
        Class(Class const&)            = delete; \
        Class& operator=(Class const&) = delete;

#define D3D11SW_NONMOVABLE(Class)                        \
        Class(Class&&) noexcept            = delete; \
        Class& operator=(Class&&) noexcept = delete;

#define D3D11SW_NONCOPYABLE_NONMOVABLE(Class) \
        D3D11SW_NONCOPYABLE(Class)            \
        D3D11SW_NONMOVABLE(Class)

#define D3D11SW_DEFAULT_COPYABLE(Class)               \
        Class(Class const&)            = default; \
        Class& operator=(Class const&) = default;

#define D3D11SW_DEFAULT_MOVABLE(Class)                   \
        Class(Class&&) noexcept            = default; \
        Class& operator=(Class&&) noexcept = default;

#define D3D11SW_DEFAULT_COPYABLE_MOVABLE(Class) \
        D3D11SW_DEFAULT_COPYABLE(Class)         \
        D3D11SW_DEFAULT_MOVABLE(Class)
