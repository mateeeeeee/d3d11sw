if(APPLE)
    macro(add_example name)
        set(EXAMPLE_SOURCES ${ARGN})
        add_executable(${name} ${EXAMPLE_SOURCES})
        target_link_libraries(${name} PRIVATE d3d11 "-framework AppKit")
        target_include_directories(${name} PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
            ${CMAKE_SOURCE_DIR}/third_party/native-windows
        )
        target_include_directories(${name} SYSTEM PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/directx-headers
        )
        target_compile_definitions(${name} PRIVATE
            D3D11SW_SHADER_DIR="${CMAKE_SOURCE_DIR}/tests/golden/shaders"
        )
    endmacro()

    add_example(triangle   triangle.mm   triangle_render.cpp)
    add_example(cube       cube.mm       cube_render.cpp)
    add_example(instanced  instanced.mm  instanced_render.cpp)
endif()
