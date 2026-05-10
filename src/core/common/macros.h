#pragma once
#include <cassert>

#define _D3DSW_STRINGIFY_IMPL(a) #a
#define _D3DSW_CONCAT_IMPL(x, y) x##y

#define D3DSW_STRINGIFY(a) _D3DSW_STRINGIFY_IMPL(a)
#define D3DSW_CONCAT(x, y) _D3DSW_CONCAT_IMPL(x, y)

#define D3DSW_TODO(...)
#define D3DSW_HACK(stmt, msg) stmt

#define D3DSW_ASSERT(expr)            assert(expr)
#define D3DSW_ASSERT_MSG(expr, msg)   assert((expr) && (msg))

#if defined(_MSC_VER)
#define D3DSW_DEBUGBREAK()       __debugbreak()
#define D3DSW_UNREACHABLE()      __assume(false)
#define D3DSW_FORCEINLINE        __forceinline
#define D3DSW_NOINLINE           __declspec(noinline)
#define D3DSW_DEBUGZONE_BEGIN    __pragma(optimize("", off))
#define D3DSW_DEBUGZONE_END      __pragma(optimize("", on))

#elif defined(__GNUC__) || defined(__clang__)
#define D3DSW_DEBUGBREAK()       __builtin_trap()
#define D3DSW_UNREACHABLE()      __builtin_unreachable()
#define D3DSW_FORCEINLINE        inline __attribute__((always_inline))
#define D3DSW_NOINLINE           __attribute__((noinline))
#define D3DSW_DEBUGZONE_BEGIN    _Pragma("GCC push_options") \
                               _Pragma("GCC optimize(\"O0\")")
#define D3DSW_DEBUGZONE_END      _Pragma("GCC pop_options")

#else
#define D3DSW_DEBUGBREAK()       ((void)0)
#define D3DSW_UNREACHABLE()      ((void)0)
#define D3DSW_FORCEINLINE        inline
#define D3DSW_NOINLINE
#define D3DSW_DEBUGZONE_BEGIN
#define D3DSW_DEBUGZONE_END
#endif

#define D3DSW_ALIGN(x, align)    ((x) & ~((align) - 1))
#define D3DSW_ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))

#define D3DSW_NODISCARD           [[nodiscard]]
#define D3DSW_NORETURN            [[noreturn]]
#define D3DSW_DEPRECATED          [[deprecated]]
#define D3DSW_MAYBE_UNUSED        [[maybe_unused]]
#define D3DSW_DEPRECATED_MSG(msg) [[deprecated(msg)]]

#if defined(_WIN32) || defined(_WIN64)
#define D3DSW_PLATFORM_WINDOWS
#elif defined(__linux__)
#define D3DSW_PLATFORM_LINUX
#elif defined(__APPLE__)
#define D3DSW_PLATFORM_MACOS
#endif

#ifdef D3DSW_PLATFORM_WINDOWS
#  ifdef D3DSW_BUILDING_DLL
#    define D3DSW_API __declspec(dllexport)
#  else
#    define D3DSW_API __declspec(dllimport)
#  endif
#else
#  define D3DSW_API
#endif

#define D3DSW_NONCOPYABLE(Class)                   \
        Class(Class const&)            = delete; \
        Class& operator=(Class const&) = delete;

#define D3DSW_NONMOVABLE(Class)                        \
        Class(Class&&) noexcept            = delete; \
        Class& operator=(Class&&) noexcept = delete;

#define D3DSW_NONCOPYABLE_NONMOVABLE(Class) \
        D3DSW_NONCOPYABLE(Class)            \
        D3DSW_NONMOVABLE(Class)

#define D3DSW_DEFAULT_COPYABLE(Class)               \
        Class(Class const&)            = default; \
        Class& operator=(Class const&) = default;

#define D3DSW_DEFAULT_MOVABLE(Class)                   \
        Class(Class&&) noexcept            = default; \
        Class& operator=(Class&&) noexcept = default;

#define D3DSW_DEFAULT_COPYABLE_MOVABLE(Class) \
        D3DSW_DEFAULT_COPYABLE(Class)         \
        D3DSW_DEFAULT_MOVABLE(Class)
