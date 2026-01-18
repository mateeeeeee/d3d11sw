#pragma once
#include "shaders/dxbc_parser.h"
#include "shaders/shader_abi.h"
#include "util/dynamic_library.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace d3d11sw {

class ShaderJIT
{
public:
    D3D11SW_API ShaderJIT();
    D3D11SW_API ~ShaderJIT();

    D3D11SW_API void* GetOrCompile(const void* bytecode, Usize len, D3D11SW_ShaderType type);
    D3D11SW_API void* FindSymbol(const void* bytecode, Usize len, const Char* name);

private:
    std::unordered_map<Uint64, void*>              _cache;
    std::vector<std::unique_ptr<DynamicLibrary>>   _handles;
    Uint64                                         _runtimeHash = 0;

private:
	std::string CacheDir() const;
	Uint64      Hash(const void* data, Usize len) const;
	Bool        WriteCpp(const std::string& path, const std::string& src) const;
	Bool        Compile(const std::string& srcPath, const std::string& libPath) const;
	void*       LoadSymbol(const std::string& libPath);
};

D3D11SW_API ShaderJIT& GetShaderJIT();

}
