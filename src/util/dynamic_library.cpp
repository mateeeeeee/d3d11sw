#include "util/dynamic_library.h"

#ifdef D3D11SW_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace d3d11sw {

DynamicLibrary::DynamicLibrary(const Char* filename)
{
    Open(filename);
}

DynamicLibrary::~DynamicLibrary()
{
    Close();
}

Bool DynamicLibrary::Open(const Char* filename)
{
    _path = filename;
#ifdef D3D11SW_PLATFORM_WINDOWS
    _handle = reinterpret_cast<void*>(LoadLibraryA(filename));
#else
    _handle = dlopen(filename, RTLD_NOW | RTLD_LOCAL);
#endif
    return _handle != nullptr;
}

void DynamicLibrary::Close()
{
    if (!IsOpen())
    {
        return;
    }

#ifdef D3D11SW_PLATFORM_WINDOWS
    FreeLibrary(static_cast<HMODULE>(_handle));
#else
    dlclose(_handle);
#endif
    _handle = nullptr;
}

void* DynamicLibrary::GetSymbolAddress(const Char* name) const
{
#ifdef D3D11SW_PLATFORM_WINDOWS
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(_handle), name));
#else
    return dlsym(_handle, name);
#endif
}

std::string DynamicLibrary::GetFilename(const Char* name)
{
#ifdef D3D11SW_PLATFORM_WINDOWS
    return std::string(name) + ".dll";
#elif defined(D3D11SW_PLATFORM_MACOS)
    return std::string(name) + ".dylib";
#else
    return std::string(name) + ".so";
#endif
}

}
