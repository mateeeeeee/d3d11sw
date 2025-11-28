#include "shaders/shader_jit.h"
#include "shaders/dxbc_parser.h"
#include "shaders/shader_codegen.h"
#include "util/dynamic_library.h"
#include "util/hash.h"
#include "common/log.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>

namespace d3d11sw {

ShaderJIT::ShaderJIT()  = default;
ShaderJIT::~ShaderJIT() = default;

ShaderJIT& GetShaderJIT()
{
    static ShaderJIT instance;
    return instance;
}

std::string ShaderJIT::CacheDir() const
{
    return "/tmp/d3d11sw/";
}

Uint64 ShaderJIT::Hash(const void* data, Usize len) const
{
    return crc64(static_cast<const Char*>(data), len);
}

Bool ShaderJIT::WriteCpp(const std::string& path, const std::string& src) const
{
    std::ofstream f(path, std::ios::out | std::ios::trunc);
    if (!f.is_open())
    {
        return false;
    }
    f << src;
    return f.good();
}

Bool ShaderJIT::Compile(const std::string& srcPath, const std::string& libPath) const
{
    std::ostringstream cmd;
    cmd << "clang++ -std=c++20 -O2 -ffast-math -march=native"
        << " -fPIC -shared"
        << " -I\"" << D3D11SW_SRC_DIR << "\""
        << " -I\"" << D3D11SW_THIRD_PARTY_DIR << "\""
        << " -I\"" << D3D11SW_THIRD_PARTY_DIR << "/native-windows\""
        << " -I\"" << D3D11SW_THIRD_PARTY_DIR << "/directx-headers\""
        << " \"" << srcPath << "\""
        << " -o \"" << libPath << "\""
        << " 2>&1";

    Int rc = std::system(cmd.str().c_str());
    return rc == 0;
}

void* ShaderJIT::LoadSymbol(const std::string& libPath)
{
    auto lib = std::make_unique<DynamicLibrary>(libPath.c_str());
    if (!lib->IsOpen())
    {
        D3D11SW_ERROR("ShaderJIT: failed to open {}", libPath.c_str());
        return nullptr;
    }

    void* fn = lib->GetSymbolAddress("ShaderMain");
    if (!fn)
    {
        D3D11SW_ERROR("ShaderJIT: ShaderMain not found in {}", libPath.c_str());
        return nullptr;
    }

    _handles.push_back(std::move(lib));
    return fn;
}

void* ShaderJIT::GetOrCompile(const void* bytecode, Usize len, D3D11SW_ShaderType type)
{
    Uint64 hash = Hash(bytecode, len);

    auto it = _cache.find(hash);
    if (it != _cache.end())
    {
        return it->second;
    }

    std::filesystem::create_directories(CacheDir());

    std::ostringstream hashStr;
    hashStr << std::hex << hash;
    std::string stem    = CacheDir() + hashStr.str();
    std::string cppPath = stem + ".cpp";
    std::string libPath = DynamicLibrary::GetFilename(stem.c_str());

    void* fn = nullptr;
    if (std::filesystem::exists(libPath))
    {
        fn = LoadSymbol(libPath);
    }

    if (!fn)
    {
        DXBCParser parser;
        D3D11SW_ParsedShader parsed;
        if (!parser.Parse(bytecode, len, parsed))
        {
            D3D11SW_ERROR("ShaderJIT: DXBC parse failed");
            _cache[hash] = nullptr;
            return nullptr;
        }
        parsed.type = type;

        std::string src = EmitShader(parsed, "shaders/shader_runtime.h");
        if (!WriteCpp(cppPath, src))
        {
            D3D11SW_ERROR("ShaderJIT: failed to write {}", cppPath.c_str());
            _cache[hash] = nullptr;
            return nullptr;
        }

        if (!Compile(cppPath, libPath))
        {
            D3D11SW_ERROR("ShaderJIT: compile failed for {}", cppPath.c_str());
            _cache[hash] = nullptr;
            return nullptr;
        }

        fn = LoadSymbol(libPath);
    }

    _cache[hash] = fn;
    return fn;
}

}
