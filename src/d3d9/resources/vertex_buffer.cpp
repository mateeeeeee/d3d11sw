#include "d3d9/resources/vertex_buffer.h"
#include "core/common/trace.h"

namespace d3dsw {

D3D9VertexBufferSW::D3D9VertexBufferSW(IDirect3DDevice9* device, UINT length, DWORD usage, DWORD fvf, D3DPOOL pool)
    : D3D9ResourceImpl(device), _size(length), _usage(usage), _fvf(fvf), _pool(pool)
{
    _data = new Uint8[length]();
}

D3D9VertexBufferSW::~D3D9VertexBufferSW()
{
    delete[] _data;
}

HRESULT STDMETHODCALLTYPE D3D9VertexBufferSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }
    if (riid == IID_IUnknown ||
        riid == IID_IDirect3DResource9 ||
        riid == IID_IDirect3DVertexBuffer9)
    {
        *ppv = static_cast<IDirect3DVertexBuffer9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

D3DRESOURCETYPE STDMETHODCALLTYPE D3D9VertexBufferSW::GetType() { return D3DRTYPE_VERTEXBUFFER; }

HRESULT STDMETHODCALLTYPE D3D9VertexBufferSW::Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD /*Flags*/)
{
    D3DSW_TRACE_MAP("IDirect3DVertexBuffer9::Lock", "Offset={}, Size={}", OffsetToLock, SizeToLock);
    if (!ppbData)
    {
        return D3DERR_INVALIDCALL;
    }
    if (OffsetToLock > _size)
    {
        return D3DERR_INVALIDCALL;
    }
    // SizeToLock==0 per D3D9: lock from OffsetToLock to end.
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

HRESULT STDMETHODCALLTYPE D3D9VertexBufferSW::Unlock()
{
    D3DSW_TRACE_MAP("IDirect3DVertexBuffer9::Unlock");
    if (!_locked)
    {
        return D3DERR_INVALIDCALL;
    }
    _locked = false;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9VertexBufferSW::GetDesc(D3DVERTEXBUFFER_DESC* pDesc)
{
    if (!pDesc)
    {
        return D3DERR_INVALIDCALL;
    }
    pDesc->Format = D3DFMT_VERTEXDATA;
    pDesc->Type   = D3DRTYPE_VERTEXBUFFER;
    pDesc->Usage  = _usage;
    pDesc->Pool   = _pool;
    pDesc->Size   = _size;
    pDesc->FVF    = _fvf;
    return S_OK;
}

}
