#pragma once
#include "core/shaders/parsed_shader.h"
#include "core/shaders/shader_abi.h"
#include "core/util/dynamic_library.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace d3dsw {

enum class D3DSW_ShaderFrontend : Uint8
{
    D3D9  = 9,
    D3D11 = 11,
};

// D3D11 passes DXBCParse, D3D9 passes SM3Parse
using ShaderParseFn = Bool(*)(const void* bytecode, Usize len, D3DSW_ParsedShader& out);

class ShaderJIT
{
public:
    D3DSW_API ShaderJIT();
    D3DSW_API ~ShaderJIT();

    D3DSW_API void* GetOrCompile(const void* bytecode, Usize len, D3DSW_ShaderType type,
                                 ShaderParseFn parse, D3DSW_ShaderFrontend frontend);
    D3DSW_API void* FindSymbol(const void* bytecode, Usize len,
                               D3DSW_ShaderFrontend frontend, const Char* name);

private:
    std::unordered_map<Uint64, void*>              _cache;
    std::vector<std::unique_ptr<DynamicLibrary>>   _handles;
    Uint64                                         _runtimeHash = 0;

private:
	std::string CacheDir() const;
	Uint64      Hash(const void* data, Usize len) const;
	Uint64      CacheKey(const void* bytecode, Usize len, D3DSW_ShaderFrontend frontend) const;
	Bool        WriteCpp(const std::string& path, const std::string& src) const;
	Bool        Compile(const std::string& srcPath, const std::string& libPath) const;
	void*       LoadSymbol(const std::string& libPath);
};

D3DSW_API ShaderJIT& GetShaderJIT();

}
