#include "core/pipeline/sw_pipeline_types.h"
#include <d3d11_4.h>

namespace d3dsw {

// Numeric compatibility asserts: the SW pipeline enums are defined to match
// D3D11 values so translation is a plain cast at the API boundary.
// These live here (a D3D11-aware TU) so the SW header itself can stay
// d3d11.h-free.

// SWComparison vs D3D11_COMPARISON_FUNC
static_assert(static_cast<Int>(SWComparison::Never)        == D3D11_COMPARISON_NEVER);
static_assert(static_cast<Int>(SWComparison::Less)         == D3D11_COMPARISON_LESS);
static_assert(static_cast<Int>(SWComparison::Equal)        == D3D11_COMPARISON_EQUAL);
static_assert(static_cast<Int>(SWComparison::LessEqual)    == D3D11_COMPARISON_LESS_EQUAL);
static_assert(static_cast<Int>(SWComparison::Greater)      == D3D11_COMPARISON_GREATER);
static_assert(static_cast<Int>(SWComparison::NotEqual)     == D3D11_COMPARISON_NOT_EQUAL);
static_assert(static_cast<Int>(SWComparison::GreaterEqual) == D3D11_COMPARISON_GREATER_EQUAL);
static_assert(static_cast<Int>(SWComparison::Always)       == D3D11_COMPARISON_ALWAYS);

// SWStencilOp vs D3D11_STENCIL_OP
static_assert(static_cast<Int>(SWStencilOp::Keep)    == D3D11_STENCIL_OP_KEEP);
static_assert(static_cast<Int>(SWStencilOp::Zero)    == D3D11_STENCIL_OP_ZERO);
static_assert(static_cast<Int>(SWStencilOp::Replace) == D3D11_STENCIL_OP_REPLACE);
static_assert(static_cast<Int>(SWStencilOp::IncrSat) == D3D11_STENCIL_OP_INCR_SAT);
static_assert(static_cast<Int>(SWStencilOp::DecrSat) == D3D11_STENCIL_OP_DECR_SAT);
static_assert(static_cast<Int>(SWStencilOp::Invert)  == D3D11_STENCIL_OP_INVERT);
static_assert(static_cast<Int>(SWStencilOp::Incr)    == D3D11_STENCIL_OP_INCR);
static_assert(static_cast<Int>(SWStencilOp::Decr)    == D3D11_STENCIL_OP_DECR);

// SWBlend vs D3D11_BLEND
static_assert(static_cast<Int>(SWBlend::Zero)           == D3D11_BLEND_ZERO);
static_assert(static_cast<Int>(SWBlend::One)            == D3D11_BLEND_ONE);
static_assert(static_cast<Int>(SWBlend::SrcColor)       == D3D11_BLEND_SRC_COLOR);
static_assert(static_cast<Int>(SWBlend::InvSrcColor)    == D3D11_BLEND_INV_SRC_COLOR);
static_assert(static_cast<Int>(SWBlend::SrcAlpha)       == D3D11_BLEND_SRC_ALPHA);
static_assert(static_cast<Int>(SWBlend::InvSrcAlpha)    == D3D11_BLEND_INV_SRC_ALPHA);
static_assert(static_cast<Int>(SWBlend::DestAlpha)      == D3D11_BLEND_DEST_ALPHA);
static_assert(static_cast<Int>(SWBlend::InvDestAlpha)   == D3D11_BLEND_INV_DEST_ALPHA);
static_assert(static_cast<Int>(SWBlend::DestColor)      == D3D11_BLEND_DEST_COLOR);
static_assert(static_cast<Int>(SWBlend::InvDestColor)   == D3D11_BLEND_INV_DEST_COLOR);
static_assert(static_cast<Int>(SWBlend::SrcAlphaSat)    == D3D11_BLEND_SRC_ALPHA_SAT);
static_assert(static_cast<Int>(SWBlend::BlendFactor)    == D3D11_BLEND_BLEND_FACTOR);
static_assert(static_cast<Int>(SWBlend::InvBlendFactor) == D3D11_BLEND_INV_BLEND_FACTOR);
static_assert(static_cast<Int>(SWBlend::Src1Color)      == D3D11_BLEND_SRC1_COLOR);
static_assert(static_cast<Int>(SWBlend::InvSrc1Color)   == D3D11_BLEND_INV_SRC1_COLOR);
static_assert(static_cast<Int>(SWBlend::Src1Alpha)      == D3D11_BLEND_SRC1_ALPHA);
static_assert(static_cast<Int>(SWBlend::InvSrc1Alpha)   == D3D11_BLEND_INV_SRC1_ALPHA);

// SWBlendOp vs D3D11_BLEND_OP
static_assert(static_cast<Int>(SWBlendOp::Add)         == D3D11_BLEND_OP_ADD);
static_assert(static_cast<Int>(SWBlendOp::Subtract)    == D3D11_BLEND_OP_SUBTRACT);
static_assert(static_cast<Int>(SWBlendOp::RevSubtract) == D3D11_BLEND_OP_REV_SUBTRACT);
static_assert(static_cast<Int>(SWBlendOp::Min)         == D3D11_BLEND_OP_MIN);
static_assert(static_cast<Int>(SWBlendOp::Max)         == D3D11_BLEND_OP_MAX);

// SWLogicOp vs D3D11_LOGIC_OP
static_assert(static_cast<Int>(SWLogicOp::Clear)        == D3D11_LOGIC_OP_CLEAR);
static_assert(static_cast<Int>(SWLogicOp::Set)          == D3D11_LOGIC_OP_SET);
static_assert(static_cast<Int>(SWLogicOp::Copy)         == D3D11_LOGIC_OP_COPY);
static_assert(static_cast<Int>(SWLogicOp::CopyInverted) == D3D11_LOGIC_OP_COPY_INVERTED);
static_assert(static_cast<Int>(SWLogicOp::Noop)         == D3D11_LOGIC_OP_NOOP);
static_assert(static_cast<Int>(SWLogicOp::Invert)       == D3D11_LOGIC_OP_INVERT);
static_assert(static_cast<Int>(SWLogicOp::And)          == D3D11_LOGIC_OP_AND);
static_assert(static_cast<Int>(SWLogicOp::Nand)         == D3D11_LOGIC_OP_NAND);
static_assert(static_cast<Int>(SWLogicOp::Or)           == D3D11_LOGIC_OP_OR);
static_assert(static_cast<Int>(SWLogicOp::Nor)          == D3D11_LOGIC_OP_NOR);
static_assert(static_cast<Int>(SWLogicOp::Xor)          == D3D11_LOGIC_OP_XOR);
static_assert(static_cast<Int>(SWLogicOp::Equiv)        == D3D11_LOGIC_OP_EQUIV);
static_assert(static_cast<Int>(SWLogicOp::AndReverse)   == D3D11_LOGIC_OP_AND_REVERSE);
static_assert(static_cast<Int>(SWLogicOp::AndInverted)  == D3D11_LOGIC_OP_AND_INVERTED);
static_assert(static_cast<Int>(SWLogicOp::OrReverse)    == D3D11_LOGIC_OP_OR_REVERSE);
static_assert(static_cast<Int>(SWLogicOp::OrInverted)   == D3D11_LOGIC_OP_OR_INVERTED);

// SWCullMode vs D3D11_CULL_MODE
static_assert(static_cast<Int>(SWCullMode::None)  == D3D11_CULL_NONE);
static_assert(static_cast<Int>(SWCullMode::Front) == D3D11_CULL_FRONT);
static_assert(static_cast<Int>(SWCullMode::Back)  == D3D11_CULL_BACK);

// SWFillMode vs D3D11_FILL_MODE
static_assert(static_cast<Int>(SWFillMode::Wireframe) == D3D11_FILL_WIREFRAME);
static_assert(static_cast<Int>(SWFillMode::Solid)     == D3D11_FILL_SOLID);

// SWDepthWriteMask vs D3D11_DEPTH_WRITE_MASK
static_assert(static_cast<Int>(SWDepthWriteMask::Zero) == D3D11_DEPTH_WRITE_MASK_ZERO);
static_assert(static_cast<Int>(SWDepthWriteMask::All)  == D3D11_DEPTH_WRITE_MASK_ALL);

// SWTextureAddress vs D3D11_TEXTURE_ADDRESS_MODE
static_assert(static_cast<Int>(SWTextureAddress::Wrap)       == D3D11_TEXTURE_ADDRESS_WRAP);
static_assert(static_cast<Int>(SWTextureAddress::Mirror)     == D3D11_TEXTURE_ADDRESS_MIRROR);
static_assert(static_cast<Int>(SWTextureAddress::Clamp)      == D3D11_TEXTURE_ADDRESS_CLAMP);
static_assert(static_cast<Int>(SWTextureAddress::Border)     == D3D11_TEXTURE_ADDRESS_BORDER);
static_assert(static_cast<Int>(SWTextureAddress::MirrorOnce) == D3D11_TEXTURE_ADDRESS_MIRROR_ONCE);

// SWInputSlotClass vs D3D11_INPUT_CLASSIFICATION
static_assert(static_cast<Int>(SWInputSlotClass::PerVertexData)   == D3D11_INPUT_PER_VERTEX_DATA);
static_assert(static_cast<Int>(SWInputSlotClass::PerInstanceData) == D3D11_INPUT_PER_INSTANCE_DATA);

// SWTopology vs D3D_PRIMITIVE_TOPOLOGY (D3D11_PRIMITIVE_TOPOLOGY is an alias)
static_assert(static_cast<Int>(SWTopology::Undefined)        == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);
static_assert(static_cast<Int>(SWTopology::PointList)        == D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
static_assert(static_cast<Int>(SWTopology::LineList)         == D3D_PRIMITIVE_TOPOLOGY_LINELIST);
static_assert(static_cast<Int>(SWTopology::LineStrip)        == D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
static_assert(static_cast<Int>(SWTopology::TriangleList)     == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
static_assert(static_cast<Int>(SWTopology::TriangleStrip)    == D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
static_assert(static_cast<Int>(SWTopology::LineListAdj)      == D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ);
static_assert(static_cast<Int>(SWTopology::LineStripAdj)     == D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ);
static_assert(static_cast<Int>(SWTopology::TriangleListAdj)  == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);
static_assert(static_cast<Int>(SWTopology::TriangleStripAdj) == D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ);
static_assert(static_cast<Int>(SWTopology::PatchList1)       == D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
static_assert(static_cast<Int>(SWTopology::PatchList32)      == D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST);

}
