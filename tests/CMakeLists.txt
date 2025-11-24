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

add_gtest(test_com             test_com.cpp)
add_gtest(test_device_creation test_device_creation.cpp)
add_gtest(test_buffer          test_buffer.cpp)
add_gtest(test_texture1d       test_texture1d.cpp)
add_gtest(test_texture2d       test_texture2d.cpp)
add_gtest(test_texture3d       test_texture3d.cpp)
add_gtest(test_check_format_support  test_check_format_support.cpp)
add_gtest(test_states          test_states.cpp)
add_gtest(test_context_state   test_context_state.cpp)
add_gtest(test_views           test_views.cpp)
add_gtest(test_dxbc_parser     test_dxbc_parser.cpp)
add_gtest(test_shader_codegen  test_shader_codegen.cpp)
add_gtest(test_shader_jit      test_shader_jit.cpp)
target_compile_definitions(test_shader_jit PRIVATE
    D3D11SW_SRC_DIR="${CMAKE_SOURCE_DIR}/src"
    D3D11SW_THIRD_PARTY_DIR="${CMAKE_SOURCE_DIR}/third_party"
)
