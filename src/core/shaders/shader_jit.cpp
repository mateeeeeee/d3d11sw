#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include "core/shaders/shader_jit.h"
#include "core/shaders/parsed_shader.h"
#include "core/shaders/shader_codegen.h"
#include "core/util/dynamic_library.h"
#include "core/util/hash.h"
#include "core/common/log.h"

namespace d3dsw {

namespace {

const Char* GetJITCompiler()
{
    if (const Char* env = std::getenv("D3DSW_JIT_COMPILER"))
    {
        return env;
    }
    return D3DSW_CXX_COMPILER;
}

#ifdef D3DSW_PLATFORM_WINDOWS
Bool IsMSVCCompiler(const Char* compiler)
{
    std::string_view sv(compiler);
    auto pos = sv.find_last_of("/\\");
    std::string_view name = (pos != std::string_view::npos) ? sv.substr(pos + 1) : sv;
    return name.starts_with("cl.") || name == "cl";
}

void EnsureMSVCEnvironment()
{
    static Bool done = false;
    if (done)
    {
        return;
    }

    done = true;
    if (std::getenv("INCLUDE"))
    {
        return;
    }

    namespace fs = std::filesystem;
    // cl.exe: <MSVC>/bin/Host<arch>/<arch>/cl.exe
    fs::path msvcRoot = fs::path(D3DSW_CXX_COMPILER).parent_path().parent_path().parent_path().parent_path();
    std::string inc = (msvcRoot / "include").string();
    std::string lib = (msvcRoot / "lib" / "x64").string();

    for (const Char* root : {"C:/Program Files (x86)", "C:/Program Files"})
    {
        fs::path kitsInc = fs::path(root) / "Windows Kits" / "10" / "Include";
        fs::path kitsLib = fs::path(root) / "Windows Kits" / "10" / "Lib";
        if (!fs::exists(kitsInc)) { continue; }
        for (const auto& e : fs::directory_iterator(kitsInc))
        {
            if (fs::exists(e.path() / "ucrt"))
            {
                inc += ";" + (e.path() / "ucrt").string();
                inc += ";" + (e.path() / "shared").string();
                inc += ";" + (e.path() / "um").string();
            }
        }
        if (fs::exists(kitsLib))
        {
            for (const auto& e : fs::directory_iterator(kitsLib))
            {
                if (fs::exists(e.path() / "ucrt" / "x64"))
                {
                    lib += ";" + (e.path() / "ucrt" / "x64").string();
                    lib += ";" + (e.path() / "um" / "x64").string();
                }
            }
        }
        break;
    }

    D3DSW_INFO("ShaderJIT: INCLUDE={}", inc);
    D3DSW_INFO("ShaderJIT: LIB={}", lib);

    _putenv_s("INCLUDE", inc.c_str());
    _putenv_s("LIB", lib.c_str());
}
#endif

}

ShaderJIT::ShaderJIT()  = default;
ShaderJIT::~ShaderJIT() = default;

ShaderJIT& GetShaderJIT()
{
    static ShaderJIT instance;
    return instance;
}

std::string ShaderJIT::CacheDir() const
{
#if defined(D3DSW_PLATFORM_WINDOWS)
    return "\\tmp\\d3dsw\\";
#else
    return "/tmp/d3dsw/"; 
#endif
    //return (std::filesystem::temp_directory_path() / "d3dsw").string() + "/";
}

Uint64 ShaderJIT::Hash(const void* data, Usize len) const
{
    return crc64(static_cast<const Char*>(data), len);
}

Uint64 ShaderJIT::CacheKey(const void* bytecode, Usize len, D3DSW_ShaderFrontend frontend) const
{
    return Hash(bytecode, len) ^ _runtimeHash ^ (Uint64(frontend) << 56);
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
    const Char* compiler = GetJITCompiler();
#ifdef D3DSW_PLATFORM_WINDOWS
    if (IsMSVCCompiler(compiler))
    {
        EnsureMSVCEnvironment();
        cmd << "\"\"" << D3DSW_CXX_COMPILER << "\""
            << " /nologo /LD"
#ifdef _DEBUG
            << " /MDd"
#else
            << " /MD"
#endif
            << " /std:c++20 /O2 /fp:fast /EHsc"
            << " /I\"" << D3DSW_SRC_DIR << "\""
            << " /I\"" << D3DSW_THIRD_PARTY_DIR << "\""
            << " \"" << srcPath << "\""
            << " \"" << D3DSW_BC_DECOMP_LIB << "\""
            << " /Fe\"" << libPath << "\""
            << " /Fo\"" << libPath << ".obj\""
            << " /link /NOIMPLIB"
            << " > \"" << logPath << "\" 2>&1\"";
    }
    else
#endif
    {
        cmd << compiler
            << " -std=c++20 -O2 -ffast-math -march=native"
            << " -fPIC -shared"
            << " -I\"" << D3DSW_SRC_DIR << "\""
            << " -I\"" << D3DSW_THIRD_PARTY_DIR << "\""
            << " \"" << srcPath << "\""
            << " \"" << D3DSW_BC_DECOMP_LIB << "\""
            << " -o \"" << libPath << "\""
            << " > \"" << logPath << "\" 2>&1";
    }
    D3DSW_INFO("ShaderJIT: {}", srcPath.c_str());
    D3DSW_INFO("ShaderJIT: cmd {}", cmd.str());
    Int rc;
#ifdef D3DSW_PLATFORM_WINDOWS
    {
        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};
        std::string cmdStr = "cmd.exe /c " + cmd.str();
        BOOL ok = CreateProcessA(nullptr, cmdStr.data(), nullptr, nullptr, FALSE,
                                 CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        rc = -1;
        if (ok)
        {
            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD exitCode = 1;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            rc = static_cast<Int>(exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
#else
    rc = std::system(cmd.str().c_str());
#endif
    if (rc != 0)
    {
        D3DSW_ERROR("ShaderJIT: compile failed (rc={}) for {}", rc, srcPath.c_str());
        std::ifstream log(logPath);
        if (log.is_open())
        {
            std::string line;
            while (std::getline(log, line))
            {
                D3DSW_ERROR("  {}", line.c_str());
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
        D3DSW_ERROR("ShaderJIT: failed to open {}", libPath.c_str());
        return nullptr;
    }

    void* fn = lib->GetSymbolAddress("ShaderMain");
    if (!fn)
    {
        D3DSW_ERROR("ShaderJIT: ShaderMain not found in {}", libPath.c_str());
        return nullptr;
    }

    _handles.push_back(std::move(lib));
    return fn;
}

void* ShaderJIT::GetOrCompile(const void* bytecode, Usize len, D3DSW_ShaderType type,
                              ShaderParseFn parse, D3DSW_ShaderFrontend frontend)
{
    if (_runtimeHash == 0)
    {
        std::string rtPath = std::string(D3DSW_SRC_DIR) + "/core/shaders/shader_runtime.h";
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

    const Uint64 hash = CacheKey(bytecode, len, frontend);
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
        D3DSW_ParsedShader parsed;
        if (!parse(bytecode, len, parsed))
        {
            D3DSW_ERROR("ShaderJIT: bytecode parse failed");
            _cache[hash] = nullptr;
            return nullptr;
        }
        parsed.type = type;
        std::string src = EmitShader(parsed, "core/shaders/shader_runtime.h");
        if (!WriteCpp(cppPath, src))
        {
            D3DSW_ERROR("ShaderJIT: failed to write {}", cppPath.c_str());
            _cache[hash] = nullptr;
            return nullptr;
        }

        if (!Compile(cppPath, libPath))
        {
            D3DSW_ERROR("ShaderJIT: compile failed for {}", cppPath.c_str());
            _cache[hash] = nullptr;
            return nullptr;
        }
        fn = LoadSymbol(libPath);
    }
    _cache[hash] = fn;
    return fn;
}

void* ShaderJIT::FindSymbol(const void* bytecode, Usize len,
                            D3DSW_ShaderFrontend frontend, const Char* name)
{
    const Uint64 hash = CacheKey(bytecode, len, frontend);
    std::ostringstream hashStr;
    hashStr << std::hex << hash;
    std::string stem    = CacheDir() + hashStr.str();
    std::string libPath = DynamicLibrary::GetFilename(stem.c_str());
    for (const auto& lib : _handles)
    {
        if (lib->GetPath() == libPath)
        {
            return lib->GetSymbolAddress(name);
        }
    }
    return nullptr;
}

}
