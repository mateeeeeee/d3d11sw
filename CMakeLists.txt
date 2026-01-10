cmake_minimum_required(VERSION 3.20)
project(d3d11sw LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(D3D11SW_LTO "Enable link-time optimization" OFF)
if(D3D11SW_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Put all .exe and .dll outputs in the same directory so the tests find our d3d11.dll before the system one.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)
