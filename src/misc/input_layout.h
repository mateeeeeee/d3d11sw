#pragma once

#include <string>
#include <vector>
#include "common/device_child_impl.h"

namespace d3d11sw {


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
