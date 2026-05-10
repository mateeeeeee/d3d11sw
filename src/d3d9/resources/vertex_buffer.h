#pragma once
#include "d3d9/common/d3d9_headers.h"
#include "d3d9/common/resource_impl.h"

namespace d3dsw {

class D3D9VertexBufferSW final : public D3D9ResourceImpl<IDirect3DVertexBuffer9>
{
public:
    D3D9VertexBufferSW(IDirect3DDevice9* device, UINT length, DWORD usage, DWORD fvf, D3DPOOL pool);
    ~D3D9VertexBufferSW() override;

    Uint8*      DataPtr()       { return _data; }
    const Uint8* DataPtr() const { return _data; }
    UINT        Size() const    { return _size; }
    DWORD       FVF()  const    { return _fvf; }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override;

    HRESULT STDMETHODCALLTYPE Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE Unlock() override;
    HRESULT STDMETHODCALLTYPE GetDesc(D3DVERTEXBUFFER_DESC* pDesc) override;

private:
    Uint8*    _data   = nullptr;
    Uint      _size   = 0;
    DWORD     _usage  = 0;
    DWORD     _fvf    = 0;
    D3DPOOL   _pool   = D3DPOOL_DEFAULT;
    Bool      _locked = false;
};

}
