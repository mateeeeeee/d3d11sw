#include "d3d9/shaders/vertex_declaration.h"
#include <cstring>

namespace d3dsw {

D3D9VertexDeclarationSW::D3D9VertexDeclarationSW(IDirect3DDevice9* device, const D3DVERTEXELEMENT9* elements)
    : _device(device)
{
    if (_device)
    {
        _device->AddRef();
    }
    if (elements)
    {
        //copy until the D3DDECL_END marker (stream=0xFF, type=UNUSED).
        const D3DVERTEXELEMENT9* p = elements;
        while (p->Stream != 0xFF || p->Type != D3DDECLTYPE_UNUSED)
        {
            _elements.push_back(*p);
            ++p;
        }
        _elements.push_back(*p);
    }
}

D3D9VertexDeclarationSW::~D3D9VertexDeclarationSW()
{
    if (_device)
    {
        _device->Release();
    }
}

HRESULT STDMETHODCALLTYPE D3D9VertexDeclarationSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }
    if (riid == IID_IUnknown || riid == IID_IDirect3DVertexDeclaration9)
    {
        *ppv = static_cast<IDirect3DVertexDeclaration9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE D3D9VertexDeclarationSW::GetDevice(IDirect3DDevice9** ppDevice)
{
    if (!ppDevice)
    {
        return D3DERR_INVALIDCALL;
    }
    *ppDevice = _device;
    if (_device)
    {
        _device->AddRef();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9VertexDeclarationSW::GetDeclaration(D3DVERTEXELEMENT9* pElement, UINT* pNumElements)
{
    if (!pNumElements)
    {
        return D3DERR_INVALIDCALL;
    }
    Uint n = static_cast<Uint>(_elements.size());
    if (!pElement)
    {
        *pNumElements = n;
        return S_OK;
    }
    if (*pNumElements < n)
    {
        *pNumElements = n;
        return D3DERR_INVALIDCALL;
    }
    std::memcpy(pElement, _elements.data(), n * sizeof(D3DVERTEXELEMENT9));
    *pNumElements = n;
    return S_OK;
}

}
