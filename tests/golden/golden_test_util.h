#pragma once

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/stat.h>

#ifndef D3D11SW_GOLDEN_DIR
#define D3D11SW_GOLDEN_DIR "."
#endif

#ifndef D3D11SW_OUTPUT_DIR
#define D3D11SW_OUTPUT_DIR "."
#endif

#ifndef D3D11SW_SHADER_DIR
#define D3D11SW_SHADER_DIR "."
#endif

inline std::vector<unsigned char> ReadBytecode(const char* path)
{
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return {};
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    if (size <= 0) { std::fclose(f); return {}; }
    std::fseek(f, 0, SEEK_SET);
    std::vector<unsigned char> data(static_cast<size_t>(size));
    if (std::fread(data.data(), 1, data.size(), f) != data.size())
    {
        std::fclose(f);
        return {};
    }
    std::fclose(f);
    return data;
}

inline void WritePPM(const char* path, const float* rgba, unsigned w, unsigned h)
{
    std::FILE* f = std::fopen(path, "wb");
    if (!f) return;
    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (unsigned i = 0; i < w * h; ++i)
    {
        unsigned char rgb[3] = {
            (unsigned char)(std::fmin(std::fmax(rgba[i * 4 + 0], 0.f), 1.f) * 255.f + 0.5f),
            (unsigned char)(std::fmin(std::fmax(rgba[i * 4 + 1], 0.f), 1.f) * 255.f + 0.5f),
            (unsigned char)(std::fmin(std::fmax(rgba[i * 4 + 2], 0.f), 1.f) * 255.f + 0.5f)
        };
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
}

inline std::vector<float> ReadPPM(const char* path, unsigned& outW, unsigned& outH)
{
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return {};

    char magic[3] = {};
    if (std::fscanf(f, "%2s", magic) != 1 || std::strcmp(magic, "P6") != 0)
    {
        std::fclose(f);
        return {};
    }
    unsigned w, h;
    if (std::fscanf(f, " %u %u", &w, &h) != 2)
    {
        std::fclose(f);
        return {};
    }
    unsigned maxVal;
    if (std::fscanf(f, " %u", &maxVal) != 1)
    {
        std::fclose(f);
        return {};
    }
    std::fgetc(f); // consume single whitespace byte after maxval

    std::vector<unsigned char> rgb(w * h * 3);
    if (std::fread(rgb.data(), 1, rgb.size(), f) != rgb.size())
    {
        std::fclose(f);
        return {};
    }
    std::fclose(f);

    outW = w;
    outH = h;
    std::vector<float> rgba(w * h * 4);
    for (unsigned i = 0; i < w * h; ++i)
    {
        rgba[i * 4 + 0] = rgb[i * 3 + 0] / 255.f;
        rgba[i * 4 + 1] = rgb[i * 3 + 1] / 255.f;
        rgba[i * 4 + 2] = rgb[i * 3 + 2] / 255.f;
        rgba[i * 4 + 3] = 1.f;
    }
    return rgba;
}

inline bool CompareExact(const float* actual, const float* expected, unsigned pixelCount)
{
    for (unsigned i = 0; i < pixelCount * 4; ++i)
    {
        if (actual[i] != expected[i]) return false;
    }
    return true;
}

inline float CompareWithTolerance(const float* actual, const float* expected, unsigned pixelCount)
{
    float maxErr = 0.f;
    for (unsigned i = 0; i < pixelCount * 4; ++i)
    {
        float err = std::fabs(actual[i] - expected[i]);
        if (err > maxErr) maxErr = err;
    }
    return maxErr;
}

inline void WriteDiffPPM(const char* path, const float* actual, const float* expected,
                          unsigned w, unsigned h)
{
    std::FILE* f = std::fopen(path, "wb");
    if (!f) return;
    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (unsigned i = 0; i < w * h; ++i)
    {
        float dr = std::fabs(actual[i * 4 + 0] - expected[i * 4 + 0]);
        float dg = std::fabs(actual[i * 4 + 1] - expected[i * 4 + 1]);
        float db = std::fabs(actual[i * 4 + 2] - expected[i * 4 + 2]);
        float maxD = std::fmax(dr, std::fmax(dg, db));
        // Red heatmap: brighter = bigger difference
        unsigned char rgb[3] = {
            (unsigned char)(std::fmin(maxD * 10.f, 1.f) * 255.f + 0.5f),
            0,
            0
        };
        std::fwrite(rgb, 1, 3, f);
    }
    std::fclose(f);
}

struct GoldenResult
{
    bool passed;
    float maxError;
    std::string message;
};

inline void MkdirP(const char* path)
{
    std::string p(path);
    for (size_t i = 1; i < p.size(); ++i)
    {
        if (p[i] == '/')
        {
            p[i] = '\0';
            mkdir(p.c_str(), 0755);
            p[i] = '/';
        }
    }
    mkdir(p.c_str(), 0755);
}

inline GoldenResult CheckGolden(const char* testName, const float* actualRGBA,
                                 unsigned width, unsigned height, float tolerance = 0.f)
{
    std::string refDir  = D3D11SW_GOLDEN_DIR;
    std::string refPath = refDir + "/" + testName + ".ppm";

    std::string outDir = D3D11SW_OUTPUT_DIR;
    MkdirP(outDir.c_str());
    WritePPM((outDir + "/" + testName + ".ppm").c_str(), actualRGBA, width, height);

    bool updateGolden = false;
    if (const char* env = std::getenv("D3D11SW_UPDATE_GOLDEN"))
        updateGolden = (std::strcmp(env, "1") == 0);

    unsigned refW = 0, refH = 0;
    std::vector<float> refData = ReadPPM(refPath.c_str(), refW, refH);

    if (refData.empty() || updateGolden)
    {
        // Generate or update reference
        MkdirP(refDir.c_str());
        WritePPM(refPath.c_str(), actualRGBA, width, height);
        if (refData.empty())
        {
            return {true, 0.f,
                    "Reference generated: " + refPath + " (first run)"};
        }
        return {true, 0.f,
                "Reference updated: " + refPath};
    }

    if (refW != width || refH != height)
    {
        return {false, 999.f,
                "Dimension mismatch: actual " + std::to_string(width) + "x" +
                std::to_string(height) + " vs reference " +
                std::to_string(refW) + "x" + std::to_string(refH)};
    }

    unsigned pixelCount = width * height;

    if (tolerance == 0.f)
    {
        // PPM round-trip comparison: quantize actual to 8-bit and compare
        std::vector<float> quantizedActual(pixelCount * 4);
        for (unsigned i = 0; i < pixelCount; ++i)
        {
            for (unsigned c = 0; c < 3; ++c)
            {
                float v = std::fmin(std::fmax(actualRGBA[i * 4 + c], 0.f), 1.f);
                quantizedActual[i * 4 + c] = std::round(v * 255.f) / 255.f;
            }
            quantizedActual[i * 4 + 3] = 1.f; // alpha stored as 1.0 in PPM
        }

        if (CompareExact(quantizedActual.data(), refData.data(), pixelCount))
        {
            return {true, 0.f, "Exact match"};
        }

        float maxErr = CompareWithTolerance(quantizedActual.data(), refData.data(), pixelCount);

        // Write failure artifacts
        WriteDiffPPM((outDir + "/" + testName + "_diff.ppm").c_str(),
                     quantizedActual.data(), refData.data(), width, height);

        return {false, maxErr,
                "Exact comparison failed (max error " + std::to_string(maxErr) +
                "). Diff written to " + outDir + "/" + testName + "_diff.ppm"};
    }

    // Tolerance comparison: quantize actual to match PPM round-trip precision
    std::vector<float> quantizedActual(pixelCount * 4);
    for (unsigned i = 0; i < pixelCount; ++i)
    {
        for (unsigned c = 0; c < 3; ++c)
        {
            float v = std::fmin(std::fmax(actualRGBA[i * 4 + c], 0.f), 1.f);
            quantizedActual[i * 4 + c] = std::round(v * 255.f) / 255.f;
        }
        quantizedActual[i * 4 + 3] = 1.f;
    }

    float maxErr = CompareWithTolerance(quantizedActual.data(), refData.data(), pixelCount);
    if (maxErr <= tolerance)
    {
        return {true, maxErr, "Passed within tolerance (max error " + std::to_string(maxErr) + ")"};
    }

    // Write failure artifacts
    WriteDiffPPM((outDir + "/" + testName + "_diff.ppm").c_str(),
                 quantizedActual.data(), refData.data(), width, height);

    return {false, maxErr,
            "Max error " + std::to_string(maxErr) + " exceeds tolerance " +
            std::to_string(tolerance) + ". Diff written to " +
            outDir + "/" + testName + "_diff.ppm"};
}
