#pragma once
#include "shaders/shader_abi.h"
#include "shaders/dxbc_parser.h"

namespace d3d11sw {

class D3D11SW_API SWDispatchExecutor
{
public:
    void DispatchCS(UINT groupCountX, UINT groupCountY, UINT groupCountZ,
                    SW_CSFn fn, SW_Resources& res,
                    const D3D11SW_ParsedShader& shader);
};

}
