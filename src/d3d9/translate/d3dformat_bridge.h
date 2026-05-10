#pragma once
#include <dxgiformat.h>
#include "d3d9/common/d3d9_headers.h"

namespace d3dsw {

DXGI_FORMAT D3DFormatToDXGI(D3DFORMAT fmt);
D3DFORMAT DXGIToD3DFormat(DXGI_FORMAT fmt);

}
