set(D3D11SW_SOURCES
    exports/dllmain.cpp
    exports/exports.cpp

    common/log.cpp
    common/common.h
    common/device_child_impl.h
    common/log.h
    common/macros.h
    common/types.h
    common/unknown_impl.h

    device/device.cpp
    device/device.h

    context/context.cpp
    context/context.h
    context/context_util.h
    context/pipeline_state.cpp
    context/pipeline_state.h
    context/dispatcher.h
    context/dispatcher.cpp
    context/rasterizer.h
    context/rasterizer.cpp
    context/depth_stencil_util.h
    context/blend_util.h
    context/shader_util.h
    context/output_merger.h
    context/output_merger.cpp
    context/vertex_pipeline.h
    context/vertex_pipeline.cpp

    resources/buffer.cpp
    resources/buffer.h
    resources/subresource_layout.h
    resources/texture_util.h
    resources/texture1d.cpp
    resources/texture1d.h
    resources/texture2d.cpp
    resources/texture2d.h
    resources/texture3d.cpp
    resources/texture3d.h

    views/render_target_view.cpp
    views/render_target_view.h
    views/depth_stencil_view.cpp
    views/depth_stencil_view.h
    views/shader_resource_view.cpp
    views/shader_resource_view.h
    views/unordered_access_view.cpp
    views/unordered_access_view.h
    views/view_util.h

    shaders/vertex_shader.cpp
    shaders/vertex_shader.h
    shaders/pixel_shader.cpp
    shaders/pixel_shader.h
    shaders/geometry_shader.cpp
    shaders/geometry_shader.h
    shaders/hull_shader.cpp
    shaders/hull_shader.h
    shaders/domain_shader.cpp
    shaders/domain_shader.h
    shaders/compute_shader.cpp
    shaders/compute_shader.h
    shaders/dxbc.h
    shaders/dxbc_parser.h
    shaders/dxbc_parser.cpp
    shaders/sm4.h
    shaders/sm4_decoder.h
    shaders/sm4_decoder.cpp
    shaders/shader_abi.h
    shaders/shader_runtime.h
    shaders/shader_codegen.h
    shaders/shader_codegen.cpp
    shaders/shader_jit.h
    shaders/shader_jit.cpp

    states/rasterizer_state.cpp
    states/rasterizer_state.h
    states/blend_state.cpp
    states/blend_state.h
    states/depth_stencil_state.cpp
    states/depth_stencil_state.h
    states/sampler_state.cpp
    states/sampler_state.h

    misc/input_layout.cpp
    misc/input_layout.h
    misc/query.cpp
    misc/query.h
    misc/command_list.cpp
    misc/command_list.h
    misc/class_linkage.cpp
    misc/class_linkage.h
    misc/fence.cpp
    misc/fence.h
    misc/multithread.cpp
    misc/multithread.h

    dxgi/factory.cpp
    dxgi/factory.h
    dxgi/adapter.cpp
    dxgi/adapter.h
    dxgi/swapchain.cpp
    dxgi/swapchain.h
    dxgi/presenter.h
    dxgi/device.cpp
    dxgi/device.h

    util/align.h
    util/enum.h
    util/format.h
    util/format.cpp
    util/fixed_point.h
    util/scratch_arena.h
    util/hash.h
    util/dynamic_library.h
    util/dynamic_library.cpp
)

if(WIN32)
    list(APPEND D3D11SW_SOURCES exports/d3d11sw.def)
    list(APPEND D3D11SW_SOURCES dxgi/presenter_win32.cpp)
elseif(APPLE)
    list(APPEND D3D11SW_SOURCES dxgi/presenter_macos.mm)
endif()

add_library(d3d11 SHARED ${D3D11SW_SOURCES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${D3D11SW_SOURCES})

target_include_directories(d3d11 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(d3d11 PRIVATE dxguid)
    set_target_properties(d3d11 PROPERTIES
        OUTPUT_NAME "d3d11"
        MODULE_DEFINITION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/exports/d3d11sw.def"
    )
    target_include_directories(d3d11 PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
    )
else()
    target_include_directories(d3d11 PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/third_party/native-windows
    )
    target_include_directories(d3d11 SYSTEM PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/directx-headers
    )
    set_target_properties(d3d11 PROPERTIES
        OUTPUT_NAME "d3d11"
    )
endif()

target_link_libraries(d3d11 PRIVATE bc_decomp)

if(APPLE)
    target_link_libraries(d3d11 PRIVATE "-framework AppKit" "-framework QuartzCore")
endif()

target_compile_options(d3d11 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/FIcommon/types.h /FIcommon/macros.h>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/common/types.h -include ${CMAKE_CURRENT_SOURCE_DIR}/common/macros.h>
)

target_compile_definitions(d3d11 PRIVATE
    D3D11SW_SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    D3D11SW_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
    D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
    D3D11SW_BC_DECOMP_LIB="$<TARGET_FILE:bc_decomp>"
    $<$<BOOL:${WIN32}>:D3D11SW_BUILDING_DLL>
)
