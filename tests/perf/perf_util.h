#pragma once

#include <d3d11_4.h>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <filesystem>

#ifdef _WIN32
#include <intrin.h>
#else
#include <sys/utsname.h>
#endif

#ifndef D3D11SW_PERF_RESULTS_DIR
#define D3D11SW_PERF_RESULTS_DIR "."
#endif

struct BenchmarkResult
{
    std::string name;
    int iterations;
    double min_ns;
    double max_ns;
    double median_ns;
    double mean_ns;
    double stddev_ns;
};

inline BenchmarkResult ComputeStats(const char* name, std::vector<double>& timings)
{
    int n = static_cast<int>(timings.size());
    std::sort(timings.begin(), timings.end());

    double sum = 0;
    for (auto t : timings)
    {
        sum += t;
    }
    double mean = sum / n;

    double variance = 0;
    for (auto t : timings)
    {
        variance += (t - mean) * (t - mean);
    }
    double stddev = std::sqrt(variance / n);

    double median = (n % 2 == 0)
        ? (timings[n / 2 - 1] + timings[n / 2]) / 2.0
        : timings[n / 2];

    return {name, n, timings.front(), timings.back(), median, mean, stddev};
}

template <typename FuncT>
BenchmarkResult RunBenchmark(const char* name, ID3D11DeviceContext* ctx, ID3D11Device* dev,
                             int iterations, int warmup, FuncT&& fn)
{
    D3D11_QUERY_DESC tsDesc{};
    tsDesc.Query = D3D11_QUERY_TIMESTAMP;

    D3D11_QUERY_DESC disjointDesc{};
    disjointDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

    ID3D11Query* disjoint = nullptr;
    dev->CreateQuery(&disjointDesc, &disjoint);

    std::vector<ID3D11Query*> tsStart(iterations);
    std::vector<ID3D11Query*> tsEnd(iterations);
    for (int i = 0; i < iterations; ++i)
    {
        dev->CreateQuery(&tsDesc, &tsStart[i]);
        dev->CreateQuery(&tsDesc, &tsEnd[i]);
    }

    for (int i = 0; i < warmup; ++i)
    {
        fn();
    }

    ctx->Begin(disjoint);
    for (int i = 0; i < iterations; ++i)
    {
        ctx->End(tsStart[i]);
        fn();
        ctx->End(tsEnd[i]);
    }
    ctx->End(disjoint);

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData{};
    ctx->GetData(disjoint, &disjointData, sizeof(disjointData), 0);

    std::vector<double> timings(iterations);
    for (int i = 0; i < iterations; ++i)
    {
        UINT64 t0 = 0, t1 = 0;
        ctx->GetData(tsStart[i], &t0, sizeof(t0), 0);
        ctx->GetData(tsEnd[i], &t1, sizeof(t1), 0);
        double deltaTicks = static_cast<double>(t1 - t0);
        timings[i] = deltaTicks * (1'000'000'000.0 / disjointData.Frequency);
    }

    for (int i = 0; i < iterations; ++i)
    {
        tsStart[i]->Release();
        tsEnd[i]->Release();
    }
    disjoint->Release();

    return ComputeStats(name, timings);
}

inline const char* FormatTime(double ns, char* buf, int bufSize)
{
    if (ns < 1000.0)
    {
        std::snprintf(buf, bufSize, "%.1f ns", ns);
    }
    else if (ns < 1000000.0)
    {
        std::snprintf(buf, bufSize, "%.1f us", ns / 1000.0);
    }
    else
    {
        std::snprintf(buf, bufSize, "%.2f ms", ns / 1000000.0);
    }
    return buf;
}

inline std::string RunCommand(const char* cmd)
{
    std::string out;
#ifdef _WIN32
    FILE* pipe = _popen(cmd, "r");
#else
    FILE* pipe = popen(cmd, "r");
#endif
    if (!pipe)
    {
        return out;
    }
    char buf[256];
    while (std::fgets(buf, sizeof(buf), pipe))
    {
        out += buf;
    }
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
    {
        out.pop_back();
    }
    return out;
}

inline std::string GetCpuName()
{
#ifdef _WIN32
    int cpuInfo[4] = {};
    char brand[49] = {};
    __cpuid(cpuInfo, 0x80000002);
    std::memcpy(brand, cpuInfo, 16);
    __cpuid(cpuInfo, 0x80000003);
    std::memcpy(brand + 16, cpuInfo, 16);
    __cpuid(cpuInfo, 0x80000004);
    std::memcpy(brand + 32, cpuInfo, 16);
    std::string cpu(brand);
    while (!cpu.empty() && cpu.back() == ' ')
    {
        cpu.pop_back();
    }
    if (!cpu.empty())
    {
        return cpu;
    }
#elif defined(__APPLE__)
    std::string cpu = RunCommand("sysctl -n machdep.cpu.brand_string 2>/dev/null");
    if (!cpu.empty())
    {
        return cpu;
    }
#else
    std::string cpu = RunCommand("cat /proc/cpuinfo 2>/dev/null | grep 'model name' | head -1 | sed 's/.*: //'");
    if (!cpu.empty())
    {
        return cpu;
    }
#endif
    return "unknown";
}

inline std::string SanitizeForFilename(const std::string& s)
{
    std::string out;
    for (char c : s)
    {
        if (c == ' ' || c == '/' || c == '\\')
        {
            out += '-';
        }
        else if (c == '(' || c == ')' || c == '@')
        {
            continue;
        }
        else
        {
            out += c;
        }
    }
    return out;
}

inline std::map<std::string, double> LoadResultsFile(const char* path)
{
    std::map<std::string, double> data;
    FILE* f = std::fopen(path, "r");
    if (!f)
    {
        return data;
    }
    char line[512];
    while (std::fgets(line, sizeof(line), f))
    {
        if (line[0] == '#' || line[0] == '\n')
        {
            continue;
        }
        char name[256];
        int iters;
        double min_us, median_us, mean_us, max_us, stddev_us;
        if (std::sscanf(line, "%255s %d %lf %lf %lf %lf %lf",
                        name, &iters, &min_us, &median_us, &mean_us, &max_us, &stddev_us) == 7)
        {
            data[name] = median_us * 1000.0;
        }
    }
    std::fclose(f);
    return data;
}

inline std::string FindLatestResultsFile()
{
    std::string cpuPrefix = SanitizeForFilename(GetCpuName()) + "_";
    std::filesystem::path dir(D3D11SW_PERF_RESULTS_DIR);
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec))
    {
        return {};
    }

    std::string best;
    std::filesystem::file_time_type bestTime{};
    for (auto& entry : std::filesystem::directory_iterator(dir, ec))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }
        std::string name = entry.path().filename().string();
        if (name.size() > cpuPrefix.size() &&
            name.substr(0, cpuPrefix.size()) == cpuPrefix)
        {
            auto mtime = entry.last_write_time(ec);
            if (!ec && (best.empty() || mtime > bestTime))
            {
                bestTime = mtime;
                best = entry.path().string();
            }
        }
    }
    return best;
}

// --- Baseline loading ---

inline std::map<std::string, double>& GetBaseline()
{
    static std::map<std::string, double> baseline = []() {
        std::string path = FindLatestResultsFile();
        if (path.empty())
        {
            return std::map<std::string, double>{};
        }
        return LoadResultsFile(path.c_str());
    }();
    return baseline;
}

inline void PrintHeader()
{
    std::printf("\n%-30s %8s %12s %12s %12s %12s %12s\n",
        "Benchmark", "Iters", "Min", "Median", "Mean", "Max", "Stddev");
    std::printf("%-30s %8s %12s %12s %12s %12s %12s\n",
        "------------------------------", "--------",
        "------------", "------------", "------------",
        "------------", "------------");
}

inline void PrintResult(const BenchmarkResult& r)
{
    char minBuf[32], medBuf[32], meanBuf[32], maxBuf[32], stdBuf[32];
    std::printf("%-30s %8d %12s %12s %12s %12s %12s\n",
        r.name.c_str(),
        r.iterations,
        FormatTime(r.min_ns, minBuf, sizeof(minBuf)),
        FormatTime(r.median_ns, medBuf, sizeof(medBuf)),
        FormatTime(r.mean_ns, meanBuf, sizeof(meanBuf)),
        FormatTime(r.max_ns, maxBuf, sizeof(maxBuf)),
        FormatTime(r.stddev_ns, stdBuf, sizeof(stdBuf)));

    auto& baseline = GetBaseline();
    auto it = baseline.find(r.name);
    if (it != baseline.end())
    {
        double prev = it->second;
        double pct = ((r.median_ns - prev) / prev) * 100.0;
        char prevBuf[32];
        FormatTime(prev, prevBuf, sizeof(prevBuf));
        if (pct > 10.0)
        {
            std::printf("  !! REGRESSION: +%.1f%% vs saved (%s)\n", pct, prevBuf);
        }
        else if (pct < -10.0)
        {
            std::printf("  >> improved: %.1f%% vs saved (%s)\n", pct, prevBuf);
        }
        else
        {
            std::printf("  vs saved: %+.1f%% (%s)\n", pct, prevBuf);
        }
    }
}

inline void PrintFooter()
{
    std::printf("%-30s %8s %12s %12s %12s %12s %12s\n\n",
        "------------------------------", "--------",
        "------------", "------------", "------------",
        "------------", "------------");
}

inline bool ShouldSaveResults()
{
    const char* env = std::getenv("D3D11SW_PERF_RESULTS");
    return env && std::strcmp(env, "1") == 0;
}

inline std::string& GetResultsFilePath()
{
    static std::string path;
    if (!path.empty())
    {
        return path;
    }

    std::string hash = RunCommand("git rev-parse --short HEAD");

    std::filesystem::path dir(D3D11SW_PERF_RESULTS_DIR);
    std::filesystem::create_directories(dir);

    path = (dir / (SanitizeForFilename(GetCpuName()) + "_" + hash + ".txt")).string();

    FILE* existing = std::fopen(path.c_str(), "r");
    if (existing)
    {
        std::fclose(existing);
    }
    else
    {
        std::string cpu = GetCpuName();
        FILE* f = std::fopen(path.c_str(), "w");
        if (f)
        {
            std::fprintf(f, "# %s  %s\n", cpu.c_str(), hash.c_str());
            std::fprintf(f, "#\n");
            std::fprintf(f, "# %-28s %4s  %14s %14s %14s %14s %14s\n",
                "benchmark", "iter", "min (us)", "median (us)", "mean (us)", "max (us)", "stddev (us)");
            std::fclose(f);
        }
    }

    return path;
}

inline void WriteResult(const BenchmarkResult& r)
{
    if (!ShouldSaveResults())
    {
        return;
    }

    auto& path = GetResultsFilePath();
    FILE* f = std::fopen(path.c_str(), "a");
    if (!f)
    {
        return;
    }
    std::fprintf(f, "%-30s %4d  %14.2f %14.2f %14.2f %14.2f %14.2f\n",
        r.name.c_str(),
        r.iterations,
        r.min_ns / 1000.0,
        r.median_ns / 1000.0,
        r.mean_ns / 1000.0,
        r.max_ns / 1000.0,
        r.stddev_ns / 1000.0);
    std::fclose(f);
}
