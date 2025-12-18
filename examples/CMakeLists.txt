if(APPLE)
    add_executable(triangle triangle.mm triangle_render.cpp)
    target_link_libraries(triangle PRIVATE d3d11 "-framework AppKit")
    target_include_directories(triangle PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/third_party/native-windows
    )
    target_include_directories(triangle SYSTEM PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/directx-headers
    )
    target_compile_definitions(triangle PRIVATE
        D3D11SW_SHADER_DIR="${CMAKE_SOURCE_DIR}/tests/golden/shaders"
    )

    add_executable(cube cube.mm cube_render.cpp)
    target_link_libraries(cube PRIVATE d3d11 "-framework AppKit")
    target_include_directories(cube PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/third_party/native-windows
    )
    target_include_directories(cube SYSTEM PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/directx-headers
    )
    target_compile_definitions(cube PRIVATE
        D3D11SW_SHADER_DIR="${CMAKE_SOURCE_DIR}/tests/golden/shaders"
    )
endif()
