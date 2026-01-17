#pragma once
#include "resources/buffer.h"
#include "resources/texture1d.h"
#include "resources/texture2d.h"
#include "resources/texture3d.h"

namespace d3d11sw {

struct D3D11SW_RESOURCE_INFO
{
    DXGI_FORMAT              Format;
    D3D11_RESOURCE_DIMENSION Dimension;
    Uint                     MipLevels;
    Uint                     ArraySize;
};

inline D3D11SW_RESOURCE_INFO GetSWResourceInfo(ID3D11Resource* pResource)
{
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&dim);
    switch (dim)
    {
        case D3D11_RESOURCE_DIMENSION_BUFFER:
            return { DXGI_FORMAT_UNKNOWN, dim, 1, 1 };
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC d{};
            static_cast<D3D11Texture1DSW*>(pResource)->GetDesc(&d);
            return { d.Format, dim, d.MipLevels, d.ArraySize };
        }
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC1 d{};
            static_cast<D3D11Texture2DSW*>(pResource)->GetDesc1(&d);
            return { d.Format, dim, d.MipLevels, d.ArraySize };
        }
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D11_TEXTURE3D_DESC1 d{};
            static_cast<D3D11Texture3DSW*>(pResource)->GetDesc1(&d);
            return { d.Format, dim, d.MipLevels, 1 };
        }
        default:
            return { DXGI_FORMAT_UNKNOWN, dim, 1, 1 };
    }
}

inline Uint8* GetSwDataPtr(ID3D11Resource* pResource, Uint subresource)
{
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&dim);
    switch (dim)
    {
        case D3D11_RESOURCE_DIMENSION_BUFFER:
        {
            D3D11BufferSW* r = static_cast<D3D11BufferSW*>(pResource);
            return static_cast<Uint8*>(r->GetDataPtr()) + r->GetSubresourceLayout(0).Offset;
        }
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11Texture1DSW* r = static_cast<D3D11Texture1DSW*>(pResource);
            return static_cast<Uint8*>(r->GetDataPtr()) + r->GetSubresourceLayout(subresource).Offset;
        }
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11Texture2DSW* r = static_cast<D3D11Texture2DSW*>(pResource);
            return static_cast<Uint8*>(r->GetDataPtr()) + r->GetSubresourceLayout(subresource).Offset;
        }
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D11Texture3DSW* r = static_cast<D3D11Texture3DSW*>(pResource);
            return static_cast<Uint8*>(r->GetDataPtr()) + r->GetSubresourceLayout(subresource).Offset;
        }
        default:
            return nullptr;
    }
}

inline D3D11SW_SUBRESOURCE_LAYOUT GetSwSubresourceLayout(ID3D11Resource* pResource, Uint subresource)
{
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&dim);
    switch (dim)
    {
        case D3D11_RESOURCE_DIMENSION_BUFFER:
            return static_cast<D3D11BufferSW*>(pResource)->GetSubresourceLayout(0);
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            return static_cast<D3D11Texture1DSW*>(pResource)->GetSubresourceLayout(subresource);
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            return static_cast<D3D11Texture2DSW*>(pResource)->GetSubresourceLayout(subresource);
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            return static_cast<D3D11Texture3DSW*>(pResource)->GetSubresourceLayout(subresource);
        default:
            return {};
    }
}

inline Uint CalcRTVSubresource(const D3D11_RENDER_TARGET_VIEW_DESC1& desc, Uint mipLevels)
{
    switch (desc.ViewDimension)
    {
        case D3D11_RTV_DIMENSION_BUFFER:           return 0;
        case D3D11_RTV_DIMENSION_TEXTURE1D:        return desc.Texture1D.MipSlice;
        case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:   return D3D11CalcSubresource(desc.Texture1DArray.MipSlice, desc.Texture1DArray.FirstArraySlice, mipLevels);
        case D3D11_RTV_DIMENSION_TEXTURE2D:        return desc.Texture2D.MipSlice;
        case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:   return D3D11CalcSubresource(desc.Texture2DArray.MipSlice, desc.Texture2DArray.FirstArraySlice, mipLevels);
        case D3D11_RTV_DIMENSION_TEXTURE2DMS:      return 0;
        case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY: return desc.Texture2DMSArray.FirstArraySlice;
        case D3D11_RTV_DIMENSION_TEXTURE3D:        return desc.Texture3D.MipSlice;
        default:                                   return 0;
    }
}

inline Uint CalcDSVSubresource(const D3D11_DEPTH_STENCIL_VIEW_DESC& desc, Uint mipLevels)
{
    switch (desc.ViewDimension)
    {
        case D3D11_DSV_DIMENSION_TEXTURE1D:        return desc.Texture1D.MipSlice;
        case D3D11_DSV_DIMENSION_TEXTURE1DARRAY:   return D3D11CalcSubresource(desc.Texture1DArray.MipSlice, desc.Texture1DArray.FirstArraySlice, mipLevels);
        case D3D11_DSV_DIMENSION_TEXTURE2D:        return desc.Texture2D.MipSlice;
        case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:   return D3D11CalcSubresource(desc.Texture2DArray.MipSlice, desc.Texture2DArray.FirstArraySlice, mipLevels);
        case D3D11_DSV_DIMENSION_TEXTURE2DMS:      return 0;
        case D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY: return desc.Texture2DMSArray.FirstArraySlice;
        default:                                   return 0;
    }
}

inline Uint CalcSRVSubresource(const D3D11_SHADER_RESOURCE_VIEW_DESC1& desc, Uint mipLevels)
{
    switch (desc.ViewDimension)
    {
        case D3D11_SRV_DIMENSION_BUFFER:           return 0;
        case D3D11_SRV_DIMENSION_TEXTURE1D:        return desc.Texture1D.MostDetailedMip;
        case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:   return D3D11CalcSubresource(desc.Texture1DArray.MostDetailedMip, desc.Texture1DArray.FirstArraySlice, mipLevels);
        case D3D11_SRV_DIMENSION_TEXTURE2D:        return desc.Texture2D.MostDetailedMip;
        case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:   return D3D11CalcSubresource(desc.Texture2DArray.MostDetailedMip, desc.Texture2DArray.FirstArraySlice, mipLevels);
        case D3D11_SRV_DIMENSION_TEXTURE2DMS:      return 0;
        case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY: return desc.Texture2DMSArray.FirstArraySlice;
        case D3D11_SRV_DIMENSION_TEXTURE3D:        return desc.Texture3D.MostDetailedMip;
        case D3D11_SRV_DIMENSION_TEXTURECUBE:      return desc.TextureCube.MostDetailedMip;
        case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY: return D3D11CalcSubresource(desc.TextureCubeArray.MostDetailedMip, desc.TextureCubeArray.First2DArrayFace, mipLevels);
        default:                                   return 0;
    }
}

inline Uint CalcUAVSubresource(const D3D11_UNORDERED_ACCESS_VIEW_DESC1& desc, Uint mipLevels)
{
    switch (desc.ViewDimension)
    {
        case D3D11_UAV_DIMENSION_BUFFER:          return 0;
        case D3D11_UAV_DIMENSION_TEXTURE1D:       return desc.Texture1D.MipSlice;
        case D3D11_UAV_DIMENSION_TEXTURE1DARRAY:  return D3D11CalcSubresource(desc.Texture1DArray.MipSlice, desc.Texture1DArray.FirstArraySlice, mipLevels);
        case D3D11_UAV_DIMENSION_TEXTURE2D:       return desc.Texture2D.MipSlice;
        case D3D11_UAV_DIMENSION_TEXTURE2DARRAY:  return D3D11CalcSubresource(desc.Texture2DArray.MipSlice, desc.Texture2DArray.FirstArraySlice, mipLevels);
        case D3D11_UAV_DIMENSION_TEXTURE3D:       return desc.Texture3D.MipSlice;
        default:                                  return 0;
    }
}


inline D3D11_RENDER_TARGET_VIEW_DESC1 MakeDefaultRTVDesc(const D3D11SW_RESOURCE_INFO& info)
{
    D3D11_RENDER_TARGET_VIEW_DESC1 desc{};
    desc.Format = info.Format;
    switch (info.Dimension)
    {
        case D3D11_RESOURCE_DIMENSION_BUFFER:
            desc.ViewDimension       = D3D11_RTV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements  = 1;
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            if (info.ArraySize > 1) 
            {
                desc.ViewDimension  = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray = { 0, 0, info.ArraySize };
            } 
            else
            {
                desc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = 0;
            }
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            if (info.ArraySize > 1) 
            {
                desc.ViewDimension  = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray = { 0, 0, info.ArraySize, 0 };
            }
            else 
            {
                desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                desc.Texture2D     = { 0, 0 };
            }
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            desc.Texture3D     = { 0, 0, (UINT)-1 };
            break;
        default:
            desc.ViewDimension = D3D11_RTV_DIMENSION_UNKNOWN;
            break;
    }
    return desc;
}

inline D3D11_DEPTH_STENCIL_VIEW_DESC MakeDefaultDSVDesc(const D3D11SW_RESOURCE_INFO& info)
{
    D3D11_DEPTH_STENCIL_VIEW_DESC desc{};
    desc.Format = info.Format;
    desc.Flags  = 0;
    switch (info.Dimension)
    {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            if (info.ArraySize > 1) 
            {
                desc.ViewDimension  = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray = { 0, 0, info.ArraySize };
            }
            else 
            {
                desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = 0;
            }
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            if (info.ArraySize > 1) 
            {
                desc.ViewDimension  = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray = { 0, 0, info.ArraySize };
            } else 
            {
                desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
                desc.Texture2D.MipSlice = 0;
            }
            break;
        default:
            desc.ViewDimension = D3D11_DSV_DIMENSION_UNKNOWN;
            break;
    }
    return desc;
}

inline D3D11_SHADER_RESOURCE_VIEW_DESC1 MakeDefaultSRVDesc(const D3D11SW_RESOURCE_INFO& info)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC1 desc{};
    desc.Format = info.Format;
    switch (info.Dimension)
    {
        case D3D11_RESOURCE_DIMENSION_BUFFER:
            desc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements  = 1;
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            if (info.ArraySize > 1) 
            {
                desc.ViewDimension  = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray = { 0, (Uint)-1, 0, info.ArraySize };
            }
            else
            {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                desc.Texture1D     = { 0, (Uint)-1 };
            }
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            if (info.ArraySize > 1) 
            {
                desc.ViewDimension  = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray = { 0, (Uint)-1, 0, info.ArraySize, 0 };
            }
            else 
            {
                desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                desc.Texture2D     = { 0, (Uint)-1, 0 };
            }
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            desc.Texture3D     = { 0, (Uint)-1 };
            break;
        default:
            desc.ViewDimension = D3D11_SRV_DIMENSION_UNKNOWN;
            break;
    }
    return desc;
}

inline D3D11_UNORDERED_ACCESS_VIEW_DESC1 MakeDefaultUAVDesc(const D3D11SW_RESOURCE_INFO& info)
{
    D3D11_UNORDERED_ACCESS_VIEW_DESC1 desc{};
    desc.Format = info.Format;
    switch (info.Dimension)
    {
        case D3D11_RESOURCE_DIMENSION_BUFFER:
            desc.ViewDimension       = D3D11_UAV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements  = 1;
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            if (info.ArraySize > 1) 
            {
                desc.ViewDimension  = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                desc.Texture1DArray = { 0, 0, info.ArraySize };
            }
            else 
            {
                desc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE1D;
                desc.Texture1D.MipSlice = 0;
            }
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            if (info.ArraySize > 1)
            {
                desc.ViewDimension  = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                desc.Texture2DArray = { 0, 0, info.ArraySize, 0 };
            } 
            else 
            {
                desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                desc.Texture2D     = { 0, 0 };
            }
            break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            desc.Texture3D     = { 0, 0, (Uint)-1 };
            break;
        default:
            desc.ViewDimension = D3D11_UAV_DIMENSION_UNKNOWN;
            break;
    }
    return desc;
}

} 
