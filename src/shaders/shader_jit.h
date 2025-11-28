#pragma once
#include "shaders/dxbc_parser.h"
#include "shaders/shader_abi.h"
#include "util/dynamic_library.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace d3d11sw {

class D3D11SW_API ShaderJIT
{
public:
    ShaderJIT();
    ~ShaderJIT();

    void* GetOrCompile(const void* bytecode, Usize len, D3D11SW_ShaderType type);

private:
    std::string CacheDir() const;
    Uint64      Hash(const void* data, Usize len) const;
    Bool        WriteCpp(const std::string& path, const std::string& src) const;
    Bool        Compile(const std::string& srcPath, const std::string& libPath) const;
    void*       LoadSymbol(const std::string& libPath);

    std::unordered_map<Uint64, void*>              _cache;
    std::vector<std::unique_ptr<DynamicLibrary>>   _handles;
};

D3D11SW_API ShaderJIT& GetShaderJIT();

}
