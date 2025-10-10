set(D3D11SW_SOURCES
    exports/dllmain.cpp
    exports/exports.cpp

    common/log.cpp

    device/device.cpp

    context/context.cpp

    resources/buffer.cpp
    resources/texture1d.cpp
    resources/texture2d.cpp
    resources/texture3d.cpp

    views/render_target_view.cpp
    views/depth_stencil_view.cpp
    views/shader_resource_view.cpp
    views/unordered_access_view.cpp

    shaders/vertex_shader.cpp
    shaders/pixel_shader.cpp
    shaders/geometry_shader.cpp
    shaders/hull_shader.cpp
    shaders/domain_shader.cpp
    shaders/compute_shader.cpp

    states/rasterizer_state.cpp
    states/blend_state.cpp
    states/depth_stencil_state.cpp
    states/sampler_state.cpp

    misc/input_layout.cpp
    misc/query.cpp
    misc/command_list.cpp
    misc/class_linkage.cpp
    misc/fence.cpp
    misc/multithread.cpp

    dxgi/factory.cpp
    dxgi/adapter.cpp
    dxgi/swapchain.cpp
    dxgi/device.cpp
)

add_library(d3d11 SHARED ${D3D11SW_SOURCES})

target_include_directories(d3d11 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(d3d11 PRIVATE dxguid)

target_compile_options(d3d11 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/FIcommon/types.h /FIcommon/macros.h>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-include common/types.h -include common/macros.h>
)

set_target_properties(d3d11 PROPERTIES
    OUTPUT_NAME "d3d11"
    MODULE_DEFINITION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/exports/d3d11sw.def"
)
