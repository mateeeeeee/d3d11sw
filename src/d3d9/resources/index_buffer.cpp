#include "d3d9/resources/index_buffer.h"

namespace d3dsw {

D3D9IndexBufferSW::D3D9IndexBufferSW(IDirect3DDevice9* device, UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool)
    : D3D9ResourceImpl(device), _size(length), _usage(usage), _format(format), _pool(pool)
{
    _data = new Uint8[length]();
}

D3D9IndexBufferSW::~D3D9IndexBufferSW()
{
    delete[] _data;
}

HRESULT STDMETHODCALLTYPE D3D9IndexBufferSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }
    if (riid == IID_IUnknown ||
        riid == IID_IDirect3DResource9 ||
        riid == IID_IDirect3DIndexBuffer9)
    {
        *ppv = static_cast<IDirect3DIndexBuffer9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

D3DRESOURCETYPE STDMETHODCALLTYPE D3D9IndexBufferSW::GetType() { return D3DRTYPE_INDEXBUFFER; }

HRESULT STDMETHODCALLTYPE D3D9IndexBufferSW::Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD /*Flags*/)
{
    if (!ppbData)
    {
        return D3DERR_INVALIDCALL;
    }
    if (OffsetToLock > _size)
    {
        return D3DERR_INVALIDCALL;
    }
    if (SizeToLock == 0)
    {
        SizeToLock = _size - OffsetToLock;
    }
    if (OffsetToLock + SizeToLock > _size)
    {
        return D3DERR_INVALIDCALL;
    }
    _locked = true;
    *ppbData = _data + OffsetToLock;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9IndexBufferSW::Unlock()
{
    if (!_locked)
    {
        return D3DERR_INVALIDCALL;
    }
    _locked = false;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9IndexBufferSW::GetDesc(D3DINDEXBUFFER_DESC* pDesc)
{
    if (!pDesc)
    {
        return D3DERR_INVALIDCALL;
    }
    pDesc->Format = _format;
    pDesc->Type   = D3DRTYPE_INDEXBUFFER;
    pDesc->Usage  = _usage;
    pDesc->Pool   = _pool;
    pDesc->Size   = _size;
    return S_OK;
}

}
