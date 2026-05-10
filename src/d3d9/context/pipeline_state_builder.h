#pragma once
#include "d3d9/context/draw_state.h"

namespace d3dsw {

struct SWPipelineState;

void BuildSWPipelineState(const D3D9SW_DRAW_STATE& in, SWPipelineState& out);

}
