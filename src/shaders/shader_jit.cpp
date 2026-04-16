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

namespace {

const Char* GetJITCompiler()
{
    if (const Char* env = std::getenv("D3D11SW_JIT_COMPILER"))
    {
        return env;
    }
    return D3D11SW_CXX_COMPILER;
}

#ifdef D3D11SW_PLATFORM_WINDOWS
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
    fs::path msvcRoot = fs::path(D3D11SW_CXX_COMPILER).parent_path().parent_path().parent_path().parent_path();
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

    D3D11SW_INFO("ShaderJIT: INCLUDE={}", inc);
    D3D11SW_INFO("ShaderJIT: LIB={}", lib);

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
#if defined(D3D11SW_PLATFORM_WINDOWS)
    return "\\tmp\\d3d11sw\\";
#else
    return "/tmp/d3d11sw/"; 
#endif
    //return (std::filesystem::temp_directory_path() / "d3d11sw").string() + "/";
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
    const Char* compiler = GetJITCompiler();
#ifdef D3D11SW_PLATFORM_WINDOWS
    if (IsMSVCCompiler(compiler))
    {
        EnsureMSVCEnvironment();
        cmd << "\"\"" << D3D11SW_CXX_COMPILER << "\""
            << " /nologo /LD"
#ifdef _DEBUG
            << " /MDd"
#else
            << " /MD"
#endif
            << " /std:c++20 /O2 /fp:fast /EHsc"
            << " /I\"" << D3D11SW_SRC_DIR << "\""
            << " /I\"" << D3D11SW_THIRD_PARTY_DIR << "\""
            << " \"" << srcPath << "\""
            << " \"" << D3D11SW_BC_DECOMP_LIB << "\""
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
            << " -I\"" << D3D11SW_SRC_DIR << "\""
            << " -I\"" << D3D11SW_THIRD_PARTY_DIR << "\""
            << " \"" << srcPath << "\""
            << " \"" << D3D11SW_BC_DECOMP_LIB << "\""
            << " -o \"" << libPath << "\""
            << " > \"" << logPath << "\" 2>&1";
    }
    D3D11SW_INFO("ShaderJIT: {}", srcPath.c_str());
    D3D11SW_INFO("ShaderJIT: cmd {}", cmd.str());
    Int rc;
#ifdef D3D11SW_PLATFORM_WINDOWS
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
        D3D11SW_ParsedShader parsed;
        if (!DXBCParse(bytecode, len, parsed))
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

void* ShaderJIT::FindSymbol(const void* bytecode, Usize len, const Char* name)
{
    const Uint64 hash = Hash(bytecode, len) ^ _runtimeHash;
    std::ostringstream hashStr;
    hashStr << std::hex << hash;
    std::string stem    = CacheDir() + hashStr.str();
    std::string libPath = DynamicLibrary::GetFilename(stem.c_str());

    for (const auto& lib : _handles)
    {
        void* fn = lib->GetSymbolAddress(name);
        if (fn)
        {
            return fn;
        }
    }
    return nullptr;
}

}
