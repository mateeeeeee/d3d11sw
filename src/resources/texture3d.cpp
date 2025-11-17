#include "resources/texture3d.h"
#include "resources/texture_util.h"

namespace d3d11sw {

HRESULT STDMETHODCALLTYPE D3D11Texture3DSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11Texture3D1))
    {
        *ppv = static_cast<ID3D11Texture3D1*>(this);
    }
    else if (riid == __uuidof(ID3D11Texture3D))
    {
        *ppv = static_cast<ID3D11Texture3D*>(this);
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

D3D11Texture3DSW::D3D11Texture3DSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

HRESULT D3D11Texture3DSW::Init(
    const D3D11_TEXTURE3D_DESC1*  pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData)
{
    if (!pDesc || pDesc->Width == 0 || pDesc->Height == 0 || pDesc->Depth == 0)
    {
        return E_INVALIDARG;
    }

    _desc = *pDesc;
    if (_desc.MipLevels == 0)
    {
        _desc.MipLevels = 1;
        for (UINT dim = std::max({_desc.Width, _desc.Height, _desc.Depth}); dim > 1; dim >>= 1)
        {
            ++_desc.MipLevels;
        }
    }

    // 3D textures: depth=Depth, arraySize=1
    BuildTextureLayouts(_desc.Format,
        _desc.Width, _desc.Height, _desc.Depth,
        _desc.MipLevels, 1,
        _layouts, _data);

    CopyInitialData(pInitialData, GetSubresourceCount(), _layouts, _data);
    return S_OK;
}

void STDMETHODCALLTYPE D3D11Texture3DSW::GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension)
{
    if (pResourceDimension)
    {
        *pResourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
    }
}

void STDMETHODCALLTYPE D3D11Texture3DSW::GetDesc(D3D11_TEXTURE3D_DESC* pDesc)
{
    if (pDesc)
    {
        std::memcpy(pDesc, &_desc, sizeof(D3D11_TEXTURE3D_DESC));
    }
}

void STDMETHODCALLTYPE D3D11Texture3DSW::GetDesc1(D3D11_TEXTURE3D_DESC1* pDesc)
{
    if (pDesc)
    {
        *pDesc = _desc;
    }
}

D3D11SW_SUBRESOURCE_LAYOUT D3D11Texture3DSW::GetSubresourceLayout(UINT Subresource) const
{
    if (Subresource < _layouts.size())
    {
        return _layouts[Subresource];
    }
    return {};
}

}
