#pragma once
#include "d3d9/common/d3d9_headers.h"
#include "core/common/common.h"
#include "core/common/unknown_impl.h"
#include <vector>

namespace d3dsw {

class D3D9VertexDeclarationSW final : public IDirect3DVertexDeclaration9, private UnknownBase
{
public:
    D3D9VertexDeclarationSW(IDirect3DDevice9* device, const D3DVERTEXELEMENT9* elements);
    ~D3D9VertexDeclarationSW() override;

    const D3DVERTEXELEMENT9* Elements() const { return _elements.data(); }
    Uint                     Count()    const { return static_cast<Uint>(_elements.size()) - 1; }  

    ULONG   STDMETHODCALLTYPE AddRef() override  { return AddRefImpl(); }
    ULONG   STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) override;
    HRESULT STDMETHODCALLTYPE GetDeclaration(D3DVERTEXELEMENT9* pElement, UINT* pNumElements) override;

private:
    IDirect3DDevice9*                _device = nullptr;
    std::vector<D3DVERTEXELEMENT9>   _elements;   
};

}
