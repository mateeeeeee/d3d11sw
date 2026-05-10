#pragma once
#include "d3d11/common/d3d11_headers.h"
#include "core/common/common.h"

namespace d3dsw {

UINT    GetFormatSupport(DXGI_FORMAT format);
HRESULT ValidateTextureBindFlags(DXGI_FORMAT format, UINT bindFlags, Bool depthStencilAllowed);

}
