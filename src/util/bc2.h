#pragma once
#include <cstdint>
#include "bc7enc/rgbcx.h"

// BC2: 16-byte block = 8 bytes explicit alpha + 8 bytes BC1 color.
// Output: 16 pixels as RGBA8 (64 bytes), same layout as rgbcx::unpack_bc1.
inline void unpack_bc2(const void* pBlock_bits, void* pPixels)
{
    const uint8_t* block = static_cast<const uint8_t*>(pBlock_bits);
    // Decode BC1 color from the second 8 bytes (ignore alpha from BC1)
    rgbcx::unpack_bc1(block + 8, pPixels, false);
    // Overwrite alpha from the first 8 bytes: 16 x 4-bit explicit alpha
    uint8_t* dst = static_cast<uint8_t*>(pPixels);
    for (uint32_t i = 0; i < 16; ++i)
    {
        uint8_t byte = block[i / 2];
        uint8_t a4 = (i & 1) ? (byte >> 4) : (byte & 0x0F);
        dst[i * 4 + 3] = (a4 << 4) | a4; // expand 4-bit to 8-bit
    }
}
