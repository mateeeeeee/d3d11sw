#pragma once
#include "d3d9/translate/sm3_ir.h"
#include "core/common/common.h"

namespace d3dsw {
// Parse an SM3 (vs_3_0 / ps_3_0) token stream into SM3DecodedShader
// Returns false on malformed input (bad version, truncated instruction, etc)
// SM1/SM2 not supported yet
Bool SM3Decode(const DWORD* tokens, Usize numTokens, SM3DecodedShader& out);
}
