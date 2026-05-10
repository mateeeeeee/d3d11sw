#pragma once
#include "d3d11/common/d3d11_headers.h"
#include <string>
#include <vector>
#include "d3d11/device/device_child_impl.h"

namespace d3dsw {


class D3D11InputLayoutSW final : public DeviceChildImpl<ID3D11InputLayout>
{
public:
    explicit D3D11InputLayoutSW(ID3D11Device* device);

    HRESULT Init(const D3D11_INPUT_ELEMENT_DESC* pElements, UINT numElements);
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    const std::vector<D3D11_INPUT_ELEMENT_DESC>& GetElements() const { return _elements; }

private:
    std::vector<std::string>             _semanticNames;
    std::vector<D3D11_INPUT_ELEMENT_DESC> _elements;
};

}
