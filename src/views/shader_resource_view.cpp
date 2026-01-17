#include "views/shader_resource_view.h"
#include "views/view_util.h"
#include "util/format.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11ShaderResourceViewSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11ShaderResourceView1))
    {
        *ppv = static_cast<ID3D11ShaderResourceView1*>(this);
    }
    else if (riid == __uuidof(ID3D11ShaderResourceView))
    {
        *ppv = static_cast<ID3D11ShaderResourceView*>(this);
    }
    else if (riid == __uuidof(ID3D11View))
    {
        *ppv = static_cast<ID3D11View*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceChild))
    {
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

D3D11ShaderResourceViewSW::D3D11ShaderResourceViewSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

D3D11ShaderResourceViewSW::~D3D11ShaderResourceViewSW()
{
    if (_resource)
    {
        _resource->Release();
    }
}

HRESULT D3D11ShaderResourceViewSW::Init(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    _resource = pResource;
    _resource->AddRef();

    D3D11SW_RESOURCE_INFO info = GetSWResourceInfo(pResource);
    _desc    = pDesc ? *pDesc : MakeDefaultSRVDesc(info);

    Uint subresource = CalcSRVSubresource(_desc, info.MipLevels);
    _dataPtr = GetSwDataPtr(pResource, subresource);
    _layout  = GetSwSubresourceLayout(pResource, subresource);
    if (_desc.Format != DXGI_FORMAT_UNKNOWN)
    {
        _layout.PixelStride = GetFormatStride(_desc.Format);
        _stride = _layout.PixelStride;
    }
    else if (info.Dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
    {
        D3D11_BUFFER_DESC bd{};
        static_cast<D3D11BufferSW*>(pResource)->GetDesc(&bd);
        _stride = bd.StructureByteStride;
    }

    Uint mostDetailedMip = 0;
    Uint mipLevels = 1;
    switch (_desc.ViewDimension)
    {
        case D3D11_SRV_DIMENSION_TEXTURE1D:
            mostDetailedMip = _desc.Texture1D.MostDetailedMip;
            mipLevels = _desc.Texture1D.MipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
            mostDetailedMip = _desc.Texture1DArray.MostDetailedMip;
            mipLevels = _desc.Texture1DArray.MipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE2D:
            mostDetailedMip = _desc.Texture2D.MostDetailedMip;
            mipLevels = _desc.Texture2D.MipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
            mostDetailedMip = _desc.Texture2DArray.MostDetailedMip;
            mipLevels = _desc.Texture2DArray.MipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURE3D:
            mostDetailedMip = _desc.Texture3D.MostDetailedMip;
            mipLevels = _desc.Texture3D.MipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURECUBE:
            mostDetailedMip = _desc.TextureCube.MostDetailedMip;
            mipLevels = _desc.TextureCube.MipLevels;
            break;
        case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
            mostDetailedMip = _desc.TextureCubeArray.MostDetailedMip;
            mipLevels = _desc.TextureCubeArray.MipLevels;
            break;
        default:
            break;
    }

    if (mipLevels == (Uint)-1)
    {
        mipLevels = info.MipLevels > mostDetailedMip ? info.MipLevels - mostDetailedMip : 1;
    }
    if (mipLevels > SW_MAX_MIP_LEVELS)
    {
        mipLevels = SW_MAX_MIP_LEVELS;
    }

    _viewMipLevels = mipLevels;
    Uint arraySlice = 0;
    switch (_desc.ViewDimension)
    {
        case D3D11_SRV_DIMENSION_TEXTURE1DARRAY: arraySlice = _desc.Texture1DArray.FirstArraySlice; break;
        case D3D11_SRV_DIMENSION_TEXTURE2DARRAY: arraySlice = _desc.Texture2DArray.FirstArraySlice; break;
        case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY: arraySlice = _desc.TextureCubeArray.First2DArrayFace; break;
        default: break;
    }

    for (Uint m = 0; m < _viewMipLevels; ++m)
    {
        Uint sub = D3D11CalcSubresource(mostDetailedMip + m, arraySlice, info.MipLevels);
        _mipLayouts[m] = GetSwSubresourceLayout(pResource, sub);
        if (_desc.Format != DXGI_FORMAT_UNKNOWN)
        {
            _mipLayouts[m].PixelStride = GetFormatStride(_desc.Format);
        }
    }

    return S_OK;
}

void STDMETHODCALLTYPE D3D11ShaderResourceViewSW::GetResource(ID3D11Resource** ppResource)
{
    if (ppResource)
    {
        *ppResource = _resource;
        if (_resource)
        {
            _resource->AddRef();
        }
    }
}

void STDMETHODCALLTYPE D3D11ShaderResourceViewSW::GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc)
{
    if (pDesc)
    {
        std::memcpy(pDesc, &_desc, sizeof(*pDesc));
    }
}

void STDMETHODCALLTYPE D3D11ShaderResourceViewSW::GetDesc1(D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = _desc;
    }
}

}
