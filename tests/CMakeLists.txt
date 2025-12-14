include(GoogleTest)

macro(add_gtest name file)
  add_executable(${name} ${file})
  target_link_libraries(${name} PRIVATE d3d11 GTest::gtest_main)
  target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src)

  if(WIN32)
    target_include_directories(${name} PRIVATE
      ${CMAKE_SOURCE_DIR}/third_party
    )
  elseif(NOT WIN32)
    target_include_directories(${name} PRIVATE
      ${CMAKE_SOURCE_DIR}/third_party
      ${CMAKE_SOURCE_DIR}/third_party/native-windows
    )
    target_include_directories(${name} SYSTEM PRIVATE
      ${CMAKE_SOURCE_DIR}/third_party/directx-headers
    )
  endif()

  target_compile_options(${name} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/FIcommon/types.h /FIcommon/macros.h>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:SHELL:-include ${CMAKE_SOURCE_DIR}/src/common/types.h -include ${CMAKE_SOURCE_DIR}/src/common/macros.h>
  )

  gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST)
endmacro()

# Unit tests
add_gtest(test_com             unit/test_com.cpp)
add_gtest(test_device_creation unit/test_device_creation.cpp)
add_gtest(test_buffer          unit/test_buffer.cpp)
add_gtest(test_texture1d       unit/test_texture1d.cpp)
add_gtest(test_texture2d       unit/test_texture2d.cpp)
add_gtest(test_texture3d       unit/test_texture3d.cpp)
add_gtest(test_check_format_support  unit/test_check_format_support.cpp)
add_gtest(test_states          unit/test_states.cpp)
add_gtest(test_context_state   unit/test_context_state.cpp)
add_gtest(test_views           unit/test_views.cpp)
add_gtest(test_dxbc_parser     unit/test_dxbc_parser.cpp)
add_gtest(test_shader_codegen  unit/test_shader_codegen.cpp)
add_gtest(test_shader_jit      unit/test_shader_jit.cpp)
add_gtest(test_shader_runtime  unit/test_shader_runtime.cpp)
add_gtest(test_shaders         unit/test_shaders.cpp)
add_gtest(test_dispatch        unit/test_dispatch.cpp)
add_gtest(test_query           unit/test_query.cpp)
add_gtest(test_resource_copies unit/test_resource_copies.cpp)
add_gtest(test_buffer_uav      unit/test_buffer_uav.cpp)
add_gtest(test_texture_formats unit/test_texture_formats.cpp)
add_gtest(test_map_modes       unit/test_map_modes.cpp)
add_gtest(test_draw_indexed    unit/test_draw_indexed.cpp)

target_compile_definitions(test_shader_jit PRIVATE
    D3D11SW_SRC_DIR="${CMAKE_SOURCE_DIR}/src"
    D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
)

# Golden tests
macro(add_golden_test name file)
  add_gtest(${name} ${file})
  target_compile_definitions(${name} PRIVATE
      D3D11SW_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/reference"
      D3D11SW_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/shaders"
      D3D11SW_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/results"
      D3D11SW_SRC_DIR="${CMAKE_SOURCE_DIR}/src"
      D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
  )
endmacro()

add_golden_test(golden_cs_gradient      golden/test_cs_gradient.cpp)
add_golden_test(golden_cs_solid_fill    golden/test_cs_solid_fill.cpp)
add_golden_test(golden_cs_checkerboard  golden/test_cs_checkerboard.cpp)
add_golden_test(golden_cs_math          golden/test_cs_math.cpp)
add_golden_test(golden_cs_shared_sum    golden/test_cs_shared_sum.cpp)
add_golden_test(golden_cs_cbuffer       golden/test_cs_cbuffer.cpp)
add_golden_test(golden_cs_multi_group   golden/test_cs_multi_group.cpp)
add_golden_test(golden_cs_loop          golden/test_cs_loop.cpp)
add_golden_test(golden_cs_structured    golden/test_cs_structured.cpp)
add_golden_test(golden_cs_sincos        golden/test_cs_sincos.cpp)
add_golden_test(golden_cs_atomic        golden/test_cs_atomic.cpp)
add_golden_test(golden_cs_kitchen_sink  golden/test_cs_kitchen_sink.cpp)
add_golden_test(golden_cs_texture       golden/test_cs_texture.cpp)
add_golden_test(golden_cs_sample_cmp    golden/test_cs_sample_cmp.cpp)
add_golden_test(golden_cs_gather        golden/test_cs_gather.cpp)
add_golden_test(golden_cs_bitwise       golden/test_cs_bitwise.cpp)
add_golden_test(golden_cs_integer_math  golden/test_cs_integer_math.cpp)
add_golden_test(golden_cs_control_flow  golden/test_cs_control_flow.cpp)

add_golden_test(golden_draw_solid_triangle  golden/test_draw_solid_triangle.cpp)
add_golden_test(golden_draw_depth           golden/test_draw_depth.cpp)
add_golden_test(golden_draw_blend           golden/test_draw_blend.cpp)
add_golden_test(golden_draw_scissor         golden/test_draw_scissor.cpp)
add_golden_test(golden_draw_gradient_triangle golden/test_draw_gradient_triangle.cpp)
add_golden_test(golden_draw_topologies       golden/test_draw_topologies.cpp)
add_golden_test(golden_draw_mrt              golden/test_draw_mrt.cpp)
add_golden_test(golden_draw_textured         golden/test_draw_textured.cpp)
add_golden_test(golden_draw_cull_modes       golden/test_draw_cull_modes.cpp)
add_golden_test(golden_draw_write_mask       golden/test_draw_write_mask.cpp)
add_golden_test(golden_draw_indirect         golden/test_draw_indirect.cpp)
add_golden_test(golden_draw_discard          golden/test_draw_discard.cpp)
add_golden_test(golden_draw_textured_cube   golden/test_draw_textured_cube.cpp)
add_golden_test(golden_draw_near_clip       golden/test_draw_near_clip.cpp)
add_golden_test(golden_draw_stencil         golden/test_draw_stencil.cpp)
add_golden_test(golden_cs_dispatch_indirect  golden/test_cs_dispatch_indirect.cpp)
add_golden_test(golden_cs_srv_structured    golden/test_cs_srv_structured.cpp)
add_golden_test(golden_cs_switch            golden/test_cs_switch.cpp)
add_golden_test(golden_draw_depth_funcs     golden/test_draw_depth_funcs.cpp)
add_golden_test(golden_draw_blend_advanced  golden/test_draw_blend_advanced.cpp)
add_golden_test(golden_draw_stencil_ops     golden/test_draw_stencil_ops.cpp)

add_gtest(test_check_feature_support  unit/test_check_feature_support.cpp)
add_gtest(test_draw_pipeline   unit/test_draw_pipeline.cpp)
add_gtest(test_format_packing  unit/test_format_packing.cpp)
add_gtest(test_clear_uav       unit/test_clear_uav.cpp)
add_gtest(test_vertex_formats  unit/test_vertex_formats.cpp)
target_compile_definitions(test_vertex_formats PRIVATE
    D3D11SW_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/shaders"
    D3D11SW_SRC_DIR="${CMAKE_SOURCE_DIR}/src"
    D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
)

# Perf tests
macro(add_perf_test name file)
  add_gtest(${name} ${file})
  target_compile_definitions(${name} PRIVATE
      D3D11SW_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/shaders"
      D3D11SW_PERF_RESULTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/perf/results"
      D3D11SW_SRC_DIR="${CMAKE_SOURCE_DIR}/src"
      D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
  )
endmacro()

add_perf_test(perf_compute  perf/perf_compute.cpp)
add_perf_test(perf_draw     perf/perf_draw.cpp)
