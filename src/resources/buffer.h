#pragma once
#include <vector>
#include "sw_resource.h"
#include "common/device_child_impl.h"

namespace d3d11sw {


class Direct3D11BufferSW : public DeviceChildImpl<ID3D11Buffer, ID3D11Resource, ID3D11DeviceChild>, public ISWResource
{
	using DeviceChildImpl::AddRef;
	using DeviceChildImpl::Release;
	using DeviceChildImpl::QueryInterface;
public:
    explicit Direct3D11BufferSW(ID3D11Device* device);

	HRESULT Init(const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData);

    void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension) override;
    void STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) override;
    UINT STDMETHODCALLTYPE GetEvictionPriority() override;

    void STDMETHODCALLTYPE GetDesc(D3D11_BUFFER_DESC* pDesc) override;

    void* GetDataPtr() override { return _data.data(); }

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
	{
        if (!ppv)
        {
             return E_POINTER;
        }

		if (riid == __uuidof(ISWResource))
		{
			*ppv = static_cast<ISWResource*>(this);
			AddRef();
			return S_OK;
		}
		return DeviceChildImpl::QueryInterface(riid, ppv);
	}

private:
	std::vector<Uint8> _data;
    D3D11_BUFFER_DESC _desc{};
};

}
