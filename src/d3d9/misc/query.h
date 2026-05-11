#pragma once
#include "d3d9/common/d3d9_headers.h"
#include "core/common/common.h"
#include "core/common/unknown_impl.h"

namespace d3dsw {

class D3D9DeviceSW;

class D3D9QuerySW final : public IDirect3DQuery9, private UnknownBase
{
public:
    D3D9QuerySW(D3D9DeviceSW* device, D3DQUERYTYPE type);

    ULONG   STDMETHODCALLTYPE AddRef()  override { return AddRefImpl(); }
    ULONG   STDMETHODCALLTYPE Release() override { return ReleaseImpl(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;

    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** ppDevice) override;
    D3DQUERYTYPE STDMETHODCALLTYPE GetType() override;
    DWORD   STDMETHODCALLTYPE GetDataSize() override;
    HRESULT STDMETHODCALLTYPE Issue(DWORD dwIssueFlags) override;
    HRESULT STDMETHODCALLTYPE GetData(void* pData, DWORD dwSize, DWORD dwGetDataFlags) override;

private:
    D3D9DeviceSW* _device    = nullptr;
    D3DQUERYTYPE  _type;
    Bool          _issued    = false;
    Uint64        _timestamp = 0;
};

}
