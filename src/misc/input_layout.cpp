#include "misc/input_layout.h"
#include "util/format.h"

namespace d3d11sw {


HRESULT STDMETHODCALLTYPE D3D11InputLayoutSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11InputLayout))
    {
        *ppv = static_cast<ID3D11InputLayout*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceChild))
    {
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

D3D11InputLayoutSW::D3D11InputLayoutSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

HRESULT D3D11InputLayoutSW::Init(const D3D11_INPUT_ELEMENT_DESC* pElements, UINT numElements)
{
    if (!pElements || numElements == 0)
    {
        return E_INVALIDARG;
    }

    _semanticNames.resize(numElements);
    _elements.resize(numElements);

    Uint slotOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
    for (Uint i = 0; i < numElements; i++)
    {
        _semanticNames[i] = pElements[i].SemanticName;
        _elements[i]      = pElements[i];
        _elements[i].SemanticName = _semanticNames[i].c_str();

        Uint slot = _elements[i].InputSlot;
        if (_elements[i].AlignedByteOffset == D3D11_APPEND_ALIGNED_ELEMENT)
        {
            _elements[i].AlignedByteOffset = slotOffsets[slot];
        }
        else
        {
            slotOffsets[slot] = _elements[i].AlignedByteOffset;
        }
        slotOffsets[slot] += GetFormatStride(_elements[i].Format);
    }
    return S_OK;
}

}
