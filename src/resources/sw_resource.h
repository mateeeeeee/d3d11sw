#pragma once
#include "common/common.h"

namespace d3d11sw {

struct __declspec(uuid("0922aec1-5941-49a2-be44-28e90784e331"))
ISWResource : public IUnknown
{
    virtual void* GetDataPtr() = 0;
};

}
