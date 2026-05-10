#include "d3d9/translate/d3dformat_bridge.h"

namespace d3dsw {

DXGI_FORMAT D3DFormatToDXGI(D3DFORMAT fmt)
{
    switch (fmt)
    {
        case D3DFMT_A8R8G8B8:         return DXGI_FORMAT_B8G8R8A8_UNORM;
        case D3DFMT_X8R8G8B8:         return DXGI_FORMAT_B8G8R8X8_UNORM;
        case D3DFMT_A8B8G8R8:         return DXGI_FORMAT_R8G8B8A8_UNORM;
        case D3DFMT_X8B8G8R8:         return DXGI_FORMAT_R8G8B8A8_UNORM;   // swizzle .a=1 at sample time
        case D3DFMT_R5G6B5:           return DXGI_FORMAT_B5G6R5_UNORM;
        case D3DFMT_A1R5G5B5:         return DXGI_FORMAT_B5G5R5A1_UNORM;
        case D3DFMT_X1R5G5B5:         return DXGI_FORMAT_B5G5R5A1_UNORM;   // caller masks alpha
        case D3DFMT_A4R4G4B4:         return DXGI_FORMAT_B4G4R4A4_UNORM;
        case D3DFMT_A8:               return DXGI_FORMAT_A8_UNORM;
        case D3DFMT_L8:               return DXGI_FORMAT_R8_UNORM;         // swizzle (r,r,r,1)
        case D3DFMT_L16:              return DXGI_FORMAT_R16_UNORM;
        case D3DFMT_A8L8:             return DXGI_FORMAT_R8G8_UNORM;       // swizzle (r,r,r,g)
        case D3DFMT_A16B16G16R16:     return DXGI_FORMAT_R16G16B16A16_UNORM;
        case D3DFMT_A16B16G16R16F:    return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case D3DFMT_A32B32G32R32F:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case D3DFMT_G16R16:           return DXGI_FORMAT_R16G16_UNORM;
        case D3DFMT_G16R16F:          return DXGI_FORMAT_R16G16_FLOAT;
        case D3DFMT_G32R32F:          return DXGI_FORMAT_R32G32_FLOAT;
        case D3DFMT_R16F:             return DXGI_FORMAT_R16_FLOAT;
        case D3DFMT_R32F:             return DXGI_FORMAT_R32_FLOAT;

        case D3DFMT_V8U8:             return DXGI_FORMAT_R8G8_SNORM;
        case D3DFMT_Q8W8V8U8:         return DXGI_FORMAT_R8G8B8A8_SNORM;
        case D3DFMT_V16U16:           return DXGI_FORMAT_R16G16_SNORM;

        case D3DFMT_DXT1:             return DXGI_FORMAT_BC1_UNORM;
        case D3DFMT_DXT2:             return DXGI_FORMAT_BC2_UNORM;        // DXT2 = DXT3 premultiplied; ignored
        case D3DFMT_DXT3:             return DXGI_FORMAT_BC2_UNORM;
        case D3DFMT_DXT4:             return DXGI_FORMAT_BC3_UNORM;        // DXT4 = DXT5 premultiplied; ignored
        case D3DFMT_DXT5:             return DXGI_FORMAT_BC3_UNORM;

        case D3DFMT_D16:              return DXGI_FORMAT_D16_UNORM;
        case D3DFMT_D24S8:            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case D3DFMT_D24X8:            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case D3DFMT_D32:              return DXGI_FORMAT_D32_FLOAT;

        case D3DFMT_INDEX16:          return DXGI_FORMAT_R16_UINT;
        case D3DFMT_INDEX32:          return DXGI_FORMAT_R32_UINT;

        case D3DFMT_R8G8B8:           return DXGI_FORMAT_B8G8R8X8_UNORM;

        case D3DFMT_R3G3B2:           
        case D3DFMT_P8:               
        case D3DFMT_UNKNOWN:
        default:
            return DXGI_FORMAT_UNKNOWN;
    }
}

D3DFORMAT DXGIToD3DFormat(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:         return D3DFMT_A8R8G8B8;
        case DXGI_FORMAT_B8G8R8X8_UNORM:         return D3DFMT_X8R8G8B8;
        case DXGI_FORMAT_R8G8B8A8_UNORM:         return D3DFMT_A8B8G8R8;
        case DXGI_FORMAT_B5G6R5_UNORM:           return D3DFMT_R5G6B5;
        case DXGI_FORMAT_B5G5R5A1_UNORM:         return D3DFMT_A1R5G5B5;
        case DXGI_FORMAT_B4G4R4A4_UNORM:         return D3DFMT_A4R4G4B4;
        case DXGI_FORMAT_A8_UNORM:               return D3DFMT_A8;
        case DXGI_FORMAT_R8_UNORM:               return D3DFMT_L8;
        case DXGI_FORMAT_R16_UNORM:              return D3DFMT_L16;
        case DXGI_FORMAT_R8G8_UNORM:             return D3DFMT_A8L8;
        case DXGI_FORMAT_R16G16B16A16_UNORM:     return D3DFMT_A16B16G16R16;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:     return D3DFMT_A16B16G16R16F;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:     return D3DFMT_A32B32G32R32F;
        case DXGI_FORMAT_R16G16_UNORM:           return D3DFMT_G16R16;
        case DXGI_FORMAT_R16G16_FLOAT:           return D3DFMT_G16R16F;
        case DXGI_FORMAT_R32G32_FLOAT:           return D3DFMT_G32R32F;
        case DXGI_FORMAT_R16_FLOAT:              return D3DFMT_R16F;
        case DXGI_FORMAT_R32_FLOAT:              return D3DFMT_R32F;
        case DXGI_FORMAT_R8G8_SNORM:             return D3DFMT_V8U8;
        case DXGI_FORMAT_R8G8B8A8_SNORM:         return D3DFMT_Q8W8V8U8;
        case DXGI_FORMAT_R16G16_SNORM:           return D3DFMT_V16U16;
        case DXGI_FORMAT_BC1_UNORM:              return D3DFMT_DXT1;
        case DXGI_FORMAT_BC2_UNORM:              return D3DFMT_DXT3;
        case DXGI_FORMAT_BC3_UNORM:              return D3DFMT_DXT5;
        case DXGI_FORMAT_D16_UNORM:              return D3DFMT_D16;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:      return D3DFMT_D24S8;
        case DXGI_FORMAT_D32_FLOAT:              return D3DFMT_D32;
        case DXGI_FORMAT_R16_UINT:               return D3DFMT_INDEX16;
        case DXGI_FORMAT_R32_UINT:               return D3DFMT_INDEX32;
        default:                                 return D3DFMT_UNKNOWN;
    }
}

}
