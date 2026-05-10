include(GoogleTest)

macro(add_d3d9_test name file)
  add_executable(${name} ${file})
  target_link_libraries(${name} PRIVATE d3d9 GTest::gtest_main)
  target_include_directories(${name} PRIVATE
      ${CMAKE_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/tests
  )

  if(WIN32)
    target_include_directories(${name} PRIVATE
      ${CMAKE_SOURCE_DIR}/third_party
    )
  else()
    target_include_directories(${name} PRIVATE
      ${CMAKE_SOURCE_DIR}/third_party
      ${CMAKE_SOURCE_DIR}/third_party/native-windows
    )
    target_include_directories(${name} SYSTEM PRIVATE
      ${CMAKE_SOURCE_DIR}/third_party/directx-headers
    )
  endif()

  target_compile_options(${name} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/FIcore/common/types.h /FIcore/common/macros.h>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:SHELL:-include ${CMAKE_SOURCE_DIR}/src/core/common/types.h -include ${CMAKE_SOURCE_DIR}/src/core/common/macros.h>
  )

  target_compile_definitions(${name} PRIVATE
    D3D9SW_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
  )

  gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST TEST_PREFIX "d3d9:")
endmacro()

add_d3d9_test(test_d3d9_create         unit/test_create9.cpp)
add_d3d9_test(test_d3d9_clear          unit/test_clear.cpp)
add_d3d9_test(test_d3d9_resources      unit/test_resources.cpp)
add_d3d9_test(test_d3d9_draw_state     unit/test_draw_state.cpp)
add_d3d9_test(test_d3d9_fvf            unit/test_fvf.cpp)
add_d3d9_test(test_d3d9_sm3_decoder    unit/test_sm3_decoder.cpp)
add_d3d9_test(test_d3d9_sm3_translator unit/test_sm3_translator.cpp)
add_d3d9_test(test_d3d9_shaders        unit/test_shaders.cpp)
add_d3d9_test(test_d3d9_render_state   unit/test_render_state.cpp)
add_d3d9_test(test_d3d9_draw_triangle  unit/test_draw_triangle.cpp)
add_d3d9_test(test_d3d9_fxc_shaders    unit/test_fxc_shaders.cpp)
add_d3d9_test(test_d3d9_texture        unit/test_texture.cpp)

# Golden tests: pixel-level reference comparison via tests/d3d9/golden/reference/.
# Sets D3D9SW_GOLDEN_DIR / D3D9SW_OUTPUT_DIR consumed by golden_test_util.h
# (which also reads D3DSW_GOLDEN_DIR / D3DSW_OUTPUT_DIR — kept identical).
macro(add_d3d9_golden_test name file)
  add_d3d9_test(${name} ${file})
  target_compile_definitions(${name} PRIVATE
      D3DSW_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/reference"
      D3DSW_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/results"
  )
endmacro()

add_d3d9_golden_test(golden_d3d9_solid_triangle      golden/test_draw_solid_triangle.cpp)
add_d3d9_golden_test(golden_d3d9_textured_triangle   golden/test_draw_textured_triangle.cpp)
add_d3d9_golden_test(golden_d3d9_opcodes             golden/test_draw_opcodes.cpp)
add_d3d9_golden_test(golden_d3d9_new_opcodes         golden/test_draw_new_opcodes.cpp)
add_d3d9_golden_test(golden_d3d9_ff_triangle         golden/test_draw_ff_triangle.cpp)
add_d3d9_golden_test(golden_d3d9_depth_blend_alpha   golden/test_draw_depth_blend_alpha.cpp)
add_d3d9_golden_test(golden_d3d9_instanced           golden/test_draw_instanced.cpp)
add_d3d9_golden_test(golden_d3d9_fog                 golden/test_draw_fog.cpp)
add_d3d9_golden_test(golden_d3d9_cube_volume         golden/test_draw_cube_volume.cpp)
