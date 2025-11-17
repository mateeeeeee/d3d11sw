include(GoogleTest)

macro(add_gtest name file)
  add_executable(${name} ${file})
  target_link_libraries(${name} PRIVATE d3d11 GTest::gtest_main)
  target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
  gtest_discover_tests(${name})
endmacro()

add_gtest(test_com             test_com.cpp)
add_gtest(test_device_creation test_device_creation.cpp)
add_gtest(test_buffer          test_buffer.cpp)
