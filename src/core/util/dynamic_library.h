#pragma once
#include "core/common/common.h"
#include <string>

namespace d3dsw {

class DynamicLibrary final
{
public:
    DynamicLibrary() = default;
    explicit DynamicLibrary(const Char* filename);
    D3DSW_NONCOPYABLE_NONMOVABLE(DynamicLibrary)
    ~DynamicLibrary();

    Bool IsOpen() const { return _handle != nullptr; }
    Bool Open(const Char* filename);
    void Close();

    void* GetSymbolAddress(const Char* name) const;
    const std::string& GetPath() const { return _path; }

    template<typename T>
    Bool GetSymbol(const Char* name, T** ptr) const
    {
        *ptr = reinterpret_cast<T*>(GetSymbolAddress(name));
        return *ptr != nullptr;
    }

    static std::string GetFilename(const Char* name);

private:
    void* _handle = nullptr;
    std::string _path;
};

}
