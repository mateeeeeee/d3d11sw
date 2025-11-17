#include "resources/texture1d.h"
#include "resources/texture_util.h"

namespace d3d11sw {

HRESULT STDMETHODCALLTYPE D3D11Texture1DSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) 
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11Texture1D))
    {
        *ppv = static_cast<ID3D11Texture1D*>(this);
    }
    else if (riid == __uuidof(ID3D11Resource))
    {
        *ppv = static_cast<ID3D11Resource*>(this);
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

D3D11Texture1DSW::D3D11Texture1DSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

HRESULT D3D11Texture1DSW::Init(
    const D3D11_TEXTURE1D_DESC*   pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData)
{
    if (!pDesc || pDesc->Width == 0)
    {
        return E_INVALIDARG;
    }

    _desc = *pDesc;
    if (_desc.MipLevels == 0)
    {
        _desc.MipLevels = 1;
        for (UINT w = _desc.Width; w > 1; w >>= 1) 
        {
            ++_desc.MipLevels;
        }
    }

    BuildTextureLayouts(_desc.Format,
        _desc.Width, 1, 1,
        _desc.MipLevels, _desc.ArraySize,
        _layouts, _data);

    CopyInitialData(pInitialData, GetSubresourceCount(), _layouts, _data);
    return S_OK;
}

void STDMETHODCALLTYPE D3D11Texture1DSW::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
    {
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE1D;
    }
}

void STDMETHODCALLTYPE D3D11Texture1DSW::GetDesc(D3D11_TEXTURE1D_DESC* pDesc)
{
    if (pDesc) 
    {
        *pDesc = _desc;
    }
}

D3D11SW_SUBRESOURCE_LAYOUT D3D11Texture1DSW::GetSubresourceLayout(UINT Subresource) const
{
    if (Subresource < _layouts.size())
    {
        return _layouts[Subresource];
    }
    return {};
}

}
