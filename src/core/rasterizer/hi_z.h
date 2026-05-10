#pragma once
#include <vector>

namespace d3dsw {

struct HiZBuffer
{
    std::vector<Float> data;
    Uint8* dsv = nullptr;
    Int tilesX = 0;
    Int tilesY = 0;
    Int width = 0;
    Int height = 0;
    Int tileSize = 0;

    void Clear(Uint8* dsvData, Int w, Int h, Int tile, Float clearDepth)
    {
        dsv = dsvData;
        width = w;
        height = h;
        tileSize = tile;
        tilesX = (w + tile - 1) / tile;
        tilesY = (h + tile - 1) / tile;
        data.assign(static_cast<Usize>(tilesX) * tilesY, clearDepth);
    }

    Bool Matches(Uint8* dsvData) const { return dsv == dsvData && !data.empty(); }

    Float Read(Int tileX, Int tileY) const
    {
        Int gCol = tileX / tileSize;
        Int gRow = tileY / tileSize;
        return data[static_cast<Usize>(gRow) * tilesX + gCol];
    }

    void Write(Int tileX, Int tileY, Float val)
    {
        Int gCol = tileX / tileSize;
        Int gRow = tileY / tileSize;
        data[static_cast<Usize>(gRow) * tilesX + gCol] = val;
    }
};

}
