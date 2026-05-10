#pragma once
#include "d3d9/common/d3d9_headers.h"
#include "d3d9/common/resource_impl.h"

namespace d3dsw {

class D3D9IndexBufferSW final : public D3D9ResourceImpl<IDirect3DIndexBuffer9>
{
public:
    D3D9IndexBufferSW(IDirect3DDevice9* device, UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool);
    ~D3D9IndexBufferSW() override;

    Uint8*      DataPtr()        { return _data; }
    const Uint8* DataPtr() const { return _data; }
    UINT        Size() const     { return _size; }
    D3DFORMAT   Format() const   { return _format; }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override;

    HRESULT STDMETHODCALLTYPE Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) override;
    HRESULT STDMETHODCALLTYPE Unlock() override;
    HRESULT STDMETHODCALLTYPE GetDesc(D3DINDEXBUFFER_DESC* pDesc) override;

private:
    Uint8*    _data   = nullptr;
    Uint      _size   = 0;
    Uint      _usage  = 0;
    D3DFORMAT _format = D3DFMT_UNKNOWN;
    D3DPOOL   _pool   = D3DPOOL_DEFAULT;
    Bool      _locked = false;
};

}
