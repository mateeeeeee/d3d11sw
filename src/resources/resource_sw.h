#pragma once
#include "common/common.h"

namespace d3d11sw {

struct D3D11SW_COM_UUID("0922aec1-5941-49a2-be44-28e90784e331")
IResourceSW : public IUnknown
{
    virtual void* GetDataPtr() = 0;
};

}

D3D11SW_DEFINE_UUID(d3d11sw::IResourceSW,
    0x0922aec1, 0x5941, 0x49a2, 0xbe, 0x44, 0x28, 0xe9, 0x07, 0x84, 0xe3, 0x31)
