# ---------------------------------------------------------------------------
# Core (API-agnostic): rasterizer, shader codegen/JIT/runtime, neutral types.
# No <d3d11*.h>. Only <dxgiformat.h> permitted among DirectX headers.
# ---------------------------------------------------------------------------
set(D3DSW_CORE_SOURCES
    core/common/common.h
    core/common/log.cpp
    core/common/log.h
    core/common/macros.h
    core/common/private_data.h
    core/common/types.h
    core/common/unknown_impl.h

    core/format/pack_util.h
    core/format/subresource_layout.h

    core/pipeline/sw_pipeline_types.h
    core/pipeline/sw_pipeline_state.h

    core/rasterizer/blend_util.h
    core/rasterizer/depth_stencil_util.h
    core/rasterizer/dispatcher.cpp
    core/rasterizer/dispatcher.h
    core/rasterizer/hi_z.h
    core/rasterizer/output_merger.cpp
    core/rasterizer/output_merger.h
    core/rasterizer/rasterizer.cpp
    core/rasterizer/rasterizer.h
    core/rasterizer/shader_util.h
    core/rasterizer/tessellator.cpp
    core/rasterizer/tessellator.h
    core/rasterizer/tile_pool.h
    core/rasterizer/vertex_pipeline.cpp
    core/rasterizer/vertex_pipeline.h

    core/shaders/parsed_shader.h
    core/shaders/shader_abi.h
    core/shaders/shader_codegen.cpp
    core/shaders/shader_codegen.h
    core/shaders/shader_constants.h
    core/shaders/shader_jit.cpp
    core/shaders/shader_jit.h
    core/shaders/shader_runtime.h
    core/shaders/sm4.h

    core/util/align.h
    core/util/bc2.h
    core/util/dynamic_library.cpp
    core/util/dynamic_library.h
    core/util/enum.h
    core/util/env.h
    core/util/fixed_point.h
    core/util/format.h
    core/util/hash.h
    core/util/scratch_arena.h
    core/util/simd_lane.h
)

# ---------------------------------------------------------------------------
# D3D11 frontend + DXGI: COM wrappers, pipeline state translator, exports.
# ---------------------------------------------------------------------------
set(D3DSW_D3D11_SOURCES
    d3d11/common/d3d11_headers.h

    d3d11/context/context.cpp
    d3d11/context/context.h
    d3d11/context/context_util.h
    d3d11/context/pipeline_state.cpp
    d3d11/context/pipeline_state.h
    d3d11/context/sw_types_compat.cpp

    d3d11/device/device.cpp
    d3d11/device/device.h
    d3d11/device/device_child_impl.h
    d3d11/device/format_support.cpp
    d3d11/device/format_support.h

    d3d11/dxgi/adapter.cpp
    d3d11/dxgi/adapter.h
    d3d11/dxgi/device.cpp
    d3d11/dxgi/device.h
    d3d11/dxgi/dxgi_object_impl.h
    d3d11/dxgi/factory.cpp
    d3d11/dxgi/factory.h
    d3d11/dxgi/output.cpp
    d3d11/dxgi/output.h
    d3d11/dxgi/swapchain.cpp
    d3d11/dxgi/swapchain.h

    presenter/presenter.h

    d3d11/exports/dllmain.cpp
    d3d11/exports/exports.cpp

    d3d11/misc/class_linkage.cpp
    d3d11/misc/class_linkage.h
    d3d11/misc/command_list.cpp
    d3d11/misc/command_list.h
    d3d11/misc/fence.cpp
    d3d11/misc/fence.h
    d3d11/misc/input_layout.cpp
    d3d11/misc/input_layout.h
    d3d11/misc/multithread.cpp
    d3d11/misc/multithread.h
    d3d11/misc/query.cpp
    d3d11/misc/query.h

    d3d11/resources/buffer.cpp
    d3d11/resources/buffer.h
    d3d11/resources/texture1d.cpp
    d3d11/resources/texture1d.h
    d3d11/resources/texture2d.cpp
    d3d11/resources/texture2d.h
    d3d11/resources/texture3d.cpp
    d3d11/resources/texture3d.h
    d3d11/resources/texture_util.h

    d3d11/shaders/compute_shader.cpp
    d3d11/shaders/compute_shader.h
    d3d11/shaders/domain_shader.cpp
    d3d11/shaders/domain_shader.h
    d3d11/shaders/dxbc.h
    d3d11/shaders/dxbc_parser.cpp
    d3d11/shaders/dxbc_parser.h
    d3d11/shaders/geometry_shader.cpp
    d3d11/shaders/geometry_shader.h
    d3d11/shaders/hull_shader.cpp
    d3d11/shaders/hull_shader.h
    d3d11/shaders/pixel_shader.cpp
    d3d11/shaders/pixel_shader.h
    d3d11/shaders/sm4_decoder.cpp
    d3d11/shaders/sm4_decoder.h
    d3d11/shaders/vertex_shader.cpp
    d3d11/shaders/vertex_shader.h

    d3d11/states/blend_state.cpp
    d3d11/states/blend_state.h
    d3d11/states/depth_stencil_state.cpp
    d3d11/states/depth_stencil_state.h
    d3d11/states/rasterizer_state.cpp
    d3d11/states/rasterizer_state.h
    d3d11/states/sampler_state.cpp
    d3d11/states/sampler_state.h

    d3d11/views/depth_stencil_view.cpp
    d3d11/views/depth_stencil_view.h
    d3d11/views/render_target_view.cpp
    d3d11/views/render_target_view.h
    d3d11/views/shader_resource_view.cpp
    d3d11/views/shader_resource_view.h
    d3d11/views/unordered_access_view.cpp
    d3d11/views/unordered_access_view.h
    d3d11/views/view_util.h
)

if(WIN32)
    list(APPEND D3DSW_D3D11_SOURCES presenter/presenter_win32.cpp)
elseif(APPLE)
    list(APPEND D3DSW_D3D11_SOURCES presenter/presenter_macos.mm)
endif()

# ---------------------------------------------------------------------------
# D3D9 frontend: COM wrappers, Direct3DCreate9, SM3 translator (later).
# ---------------------------------------------------------------------------
set(D3DSW_D3D9_SOURCES
    d3d9/common/d3d9_headers.h
    d3d9/common/resource_impl.h

    d3d9/context/draw_state.h
    d3d9/context/pipeline_state_builder.cpp
    d3d9/context/pipeline_state_builder.h

    d3d9/ff/ff_synthesizer.cpp
    d3d9/ff/ff_synthesizer.h

    d3d9/state_block/state_block.cpp
    d3d9/state_block/state_block.h

    d3d9/misc/query.cpp
    d3d9/misc/query.h

    d3d9/device/device.cpp
    d3d9/device/device.h

    d3d9/factory/direct3d9.cpp
    d3d9/factory/direct3d9.h

    d3d9/resources/cube_texture.cpp
    d3d9/resources/cube_texture.h
    d3d9/resources/index_buffer.cpp
    d3d9/resources/index_buffer.h
    d3d9/resources/surface.cpp
    d3d9/resources/surface.h
    d3d9/resources/texture.cpp
    d3d9/resources/texture.h
    d3d9/resources/vertex_buffer.cpp
    d3d9/resources/vertex_buffer.h
    d3d9/resources/volume.cpp
    d3d9/resources/volume.h
    d3d9/resources/volume_texture.cpp
    d3d9/resources/volume_texture.h

    d3d9/shaders/vertex_declaration.cpp
    d3d9/shaders/vertex_declaration.h
    d3d9/shaders/vertex_shader.cpp
    d3d9/shaders/vertex_shader.h
    d3d9/shaders/pixel_shader.cpp
    d3d9/shaders/pixel_shader.h

    d3d9/swapchain/swap_chain.cpp
    d3d9/swapchain/swap_chain.h

    d3d9/translate/d3dformat_bridge.cpp
    d3d9/translate/d3dformat_bridge.h
    d3d9/translate/fvf.cpp
    d3d9/translate/fvf.h
    d3d9/translate/sm3_decoder.cpp
    d3d9/translate/sm3_decoder.h
    d3d9/translate/sm3_ir.h
    d3d9/translate/sm3_translator.cpp
    d3d9/translate/sm3_translator.h

    d3d9/exports/dllmain.cpp
    d3d9/exports/d3d9_exports.cpp
)

# ---------------------------------------------------------------------------
# OBJECT libraries: one per layer. Each .dll below links whichever it needs.
#   d3dsw_core         API-neutral rasterizer + shaders (+ OS presenter)
#   d3dsw_d3d11        D3D11 frontend (incl. DXGI sources)
#   d3dsw_d3d9         D3D9 frontend
# ---------------------------------------------------------------------------
if(WIN32)
    list(APPEND D3DSW_CORE_SOURCES presenter/presenter_win32.cpp)
elseif(APPLE)
    list(APPEND D3DSW_CORE_SOURCES presenter/presenter_macos.mm)
endif()
list(REMOVE_ITEM D3DSW_D3D11_SOURCES
    presenter/presenter_win32.cpp
    presenter/presenter_macos.mm
)

add_library(d3dsw_core  OBJECT ${D3DSW_CORE_SOURCES})
add_library(d3dsw_d3d11 OBJECT ${D3DSW_D3D11_SOURCES})
add_library(d3dsw_d3d9  OBJECT ${D3DSW_D3D9_SOURCES})

foreach(target d3dsw_core d3dsw_d3d11 d3dsw_d3d9)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    if(WIN32)
        target_include_directories(${target} PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party
        )
    else()
        target_include_directories(${target} PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party
            ${CMAKE_SOURCE_DIR}/third_party/native-windows
        )
        target_include_directories(${target} SYSTEM PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/directx-headers
        )
    endif()

    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/FIcore/common/types.h /FIcore/common/macros.h>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/core/common/types.h -include ${CMAKE_CURRENT_SOURCE_DIR}/core/common/macros.h>
    )

    target_compile_definitions(${target} PRIVATE
        D3DSW_SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        D3DSW_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
        D3DSW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
        D3DSW_BC_DECOMP_LIB="$<TARGET_FILE:bc_decomp>"
        $<$<BOOL:${WIN32}>:D3DSW_BUILDING_DLL>
    )
endforeach()

# d3d11.dll
if(WIN32)
    add_library(d3d11 SHARED
        $<TARGET_OBJECTS:d3dsw_core>
        $<TARGET_OBJECTS:d3dsw_d3d11>
        d3d11/exports/d3d11sw.def)
else()
    add_library(d3d11 SHARED
        $<TARGET_OBJECTS:d3dsw_core>
        $<TARGET_OBJECTS:d3dsw_d3d11>)
endif()

if(WIN32)
    target_link_libraries(d3d11 PRIVATE dxguid)
    set_target_properties(d3d11 PROPERTIES
        OUTPUT_NAME "d3d11"
        MODULE_DEFINITION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/d3d11/exports/d3d11sw.def"
    )
else()
    set_target_properties(d3d11 PROPERTIES
        OUTPUT_NAME "d3d11"
    )
endif()

target_link_libraries(d3d11 PRIVATE bc_decomp tessellator)

if(APPLE)
    target_link_libraries(d3d11 PRIVATE "-framework AppKit" "-framework QuartzCore")
endif()

# dxgi.dll
set(DXGI_SOURCES d3d11/exports/dxgi_exports.cpp)
if(WIN32)
    list(APPEND DXGI_SOURCES d3d11/exports/dxgi.def)
endif()

add_library(dxgi SHARED
    ${DXGI_SOURCES}
    $<TARGET_OBJECTS:d3dsw_core>
    $<TARGET_OBJECTS:d3dsw_d3d11>)

target_include_directories(dxgi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(dxgi PRIVATE dxguid)
    target_include_directories(dxgi PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
    )
    set_target_properties(dxgi PROPERTIES
        OUTPUT_NAME "dxgi"
        MODULE_DEFINITION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/d3d11/exports/dxgi.def"
    )
else()
    target_include_directories(dxgi PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/third_party/native-windows
    )
    target_include_directories(dxgi SYSTEM PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/directx-headers
    )
    set_target_properties(dxgi PROPERTIES
        OUTPUT_NAME "dxgi"
    )
endif()

target_link_libraries(dxgi PRIVATE bc_decomp tessellator)

if(APPLE)
    target_link_libraries(dxgi PRIVATE "-framework AppKit" "-framework QuartzCore")
endif()

target_compile_options(dxgi PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/FIcore/common/types.h /FIcore/common/macros.h>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/core/common/types.h -include ${CMAKE_CURRENT_SOURCE_DIR}/core/common/macros.h>
)

target_compile_definitions(dxgi PRIVATE
    D3DSW_SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    D3DSW_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
    D3DSW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
    D3DSW_BC_DECOMP_LIB="$<TARGET_FILE:bc_decomp>"
    $<$<BOOL:${WIN32}>:D3DSW_BUILDING_DLL>
)

# d3d9.dll
if(WIN32)
    add_library(d3d9 SHARED
        $<TARGET_OBJECTS:d3dsw_core>
        $<TARGET_OBJECTS:d3dsw_d3d9>
        d3d9/exports/d3d9.def)
else()
    add_library(d3d9 SHARED
        $<TARGET_OBJECTS:d3dsw_core>
        $<TARGET_OBJECTS:d3dsw_d3d9>)
endif()

if(WIN32)
    set_target_properties(d3d9 PROPERTIES
        OUTPUT_NAME "d3d9"
        MODULE_DEFINITION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/d3d9/exports/d3d9.def"
    )
else()
    set_target_properties(d3d9 PROPERTIES
        OUTPUT_NAME "d3d9"
    )
endif()

target_link_libraries(d3d9 PRIVATE bc_decomp tessellator)

if(APPLE)
    target_link_libraries(d3d9 PRIVATE "-framework AppKit" "-framework QuartzCore")
endif()
