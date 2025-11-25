include(GoogleTest)

macro(add_gtest name file)
  add_executable(${name} ${file})
  target_link_libraries(${name} PRIVATE d3d11 GTest::gtest_main)
  target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src)

  if(NOT WIN32)
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

  gtest_discover_tests(${name})
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
target_compile_definitions(test_shader_jit PRIVATE
    D3D11SW_SRC_DIR="${CMAKE_SOURCE_DIR}/src"
    D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
)
add_gtest(test_shaders         unit/test_shaders.cpp)
add_gtest(test_dispatch        unit/test_dispatch.cpp)
add_gtest(test_query           unit/test_query.cpp)

# Golden tests
add_gtest(golden_cs_gradient   golden/test_cs_gradient.cpp)
target_compile_definitions(golden_cs_gradient PRIVATE
    D3D11SW_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/reference"
    D3D11SW_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/shaders"
    D3D11SW_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/results"
    D3D11SW_SRC_DIR="${CMAKE_SOURCE_DIR}/src"
    D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
)

add_gtest(golden_cs_solid_fill     golden/test_cs_solid_fill.cpp)
target_compile_definitions(golden_cs_solid_fill PRIVATE
    D3D11SW_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/reference"
    D3D11SW_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/shaders"
    D3D11SW_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/results"
    D3D11SW_SRC_DIR="${CMAKE_SOURCE_DIR}/src"
    D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
)

add_gtest(golden_cs_checkerboard   golden/test_cs_checkerboard.cpp)
target_compile_definitions(golden_cs_checkerboard PRIVATE
    D3D11SW_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/reference"
    D3D11SW_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/shaders"
    D3D11SW_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/results"
    D3D11SW_SRC_DIR="${CMAKE_SOURCE_DIR}/src"
    D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
)

add_gtest(golden_cs_math           golden/test_cs_math.cpp)
target_compile_definitions(golden_cs_math PRIVATE
    D3D11SW_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/reference"
    D3D11SW_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/shaders"
    D3D11SW_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/results"
    D3D11SW_SRC_DIR="${CMAKE_SOURCE_DIR}/src"
    D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
)

add_gtest(golden_cs_shared_sum     golden/test_cs_shared_sum.cpp)
target_compile_definitions(golden_cs_shared_sum PRIVATE
    D3D11SW_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/reference"
    D3D11SW_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/shaders"
    D3D11SW_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden/results"
    D3D11SW_SRC_DIR="${CMAKE_SOURCE_DIR}/src"
    D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
)
