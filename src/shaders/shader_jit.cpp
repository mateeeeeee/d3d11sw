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
    return "/tmp/d3d11sw/"; //return (std::filesystem::temp_directory_path() / "d3d11sw").string() + "/";
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
    std::string logPath = libPath + ".log";
    std::ostringstream cmd;
#ifdef _MSC_VER
    cmd << "\"\"" << D3D11SW_CXX_COMPILER << "\""
        << " /nologo /LD /MD /std:c++20 /O2 /fp:precise /EHsc"
        << " /I\"" << D3D11SW_SRC_DIR << "\""
        << " /I\"" << D3D11SW_THIRD_PARTY_DIR << "\""
        << " \"" << srcPath << "\""
        << " /Fe\"" << libPath << "\""
        << " /Fo\"" << libPath << ".obj\""
        << " /link /NOIMPLIB\""
        << " > \"" << logPath << "\" 2>&1";
#else
    cmd << "clang++"
        << " -std=c++20 -O2 -ffast-math -march=native"
        << " -fPIC -shared"
        << " -I\"" << D3D11SW_SRC_DIR << "\""
        << " -I\"" << D3D11SW_THIRD_PARTY_DIR << "\""
        << " -I\"" << D3D11SW_THIRD_PARTY_DIR << "/native-windows\""
        << " -I\"" << D3D11SW_THIRD_PARTY_DIR << "/directx-headers\""
        << " \"" << srcPath << "\""
        << " -o \"" << libPath << "\""
        << " > \"" << logPath << "\" 2>&1";
#endif
    D3D11SW_INFO("ShaderJIT: {}", srcPath.c_str());
    Int rc = std::system(cmd.str().c_str());
    if (rc != 0)
    {
        D3D11SW_ERROR("ShaderJIT: compile failed (rc={}) for {}", rc, srcPath.c_str());
        std::ifstream log(logPath);
        if (log.is_open())
        {
            std::string line;
            while (std::getline(log, line))
            {
                D3D11SW_ERROR("  {}", line.c_str());
            }
        }
        return false;
    }
    return true;
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
    if (_runtimeHash == 0)
    {
        std::string rtPath = std::string(D3D11SW_SRC_DIR) + "/shaders/shader_runtime.h";
        std::ifstream rtFile(rtPath, std::ios::binary);
        if (rtFile.is_open())
        {
            std::string rtSrc((std::istreambuf_iterator<char>(rtFile)),
                               std::istreambuf_iterator<char>());
            _runtimeHash = Hash(rtSrc.data(), rtSrc.size());
        }
        if (_runtimeHash == 0)
        {
            _runtimeHash = 1;
        }
    }

    const Uint64 hash = Hash(bytecode, len) ^ _runtimeHash;
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
