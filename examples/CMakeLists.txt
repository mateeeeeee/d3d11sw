if(APPLE)
    macro(add_example name)
        set(EXAMPLE_SOURCES ${ARGN})
        add_executable(${name} ${EXAMPLE_SOURCES})
        target_link_libraries(${name} PRIVATE d3d11 "-framework AppKit")
        target_include_directories(${name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
            ${CMAKE_SOURCE_DIR}/third_party/native-windows
        )
        target_include_directories(${name} SYSTEM PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/directx-headers
        )
        target_compile_definitions(${name} PRIVATE
            D3D11SW_SHADER_DIR="${CMAKE_SOURCE_DIR}/tests/shaders"
        )
    endmacro()

    add_example(triangle     macos/triangle.mm     triangle_render.cpp)
    add_example(cube         macos/cube.mm         cube_render.cpp)
    add_example(instanced    macos/instanced.mm    instanced_render.cpp)
    add_example(postprocess  macos/postprocess.mm  postprocess_render.cpp)
    add_example(floor        macos/floor.mm        floor_render.cpp)
elseif(WIN32)
    macro(add_example name)
        set(EXAMPLE_SOURCES ${ARGN})
        add_executable(${name} ${EXAMPLE_SOURCES})
        target_link_libraries(${name} PRIVATE d3d11)
        set_target_properties(${name} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")
        target_include_directories(${name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/win32
            ${CMAKE_SOURCE_DIR}/src
        )
        target_compile_definitions(${name} PRIVATE
            D3D11SW_SHADER_DIR="${CMAKE_SOURCE_DIR}/tests/shaders"
        )
    endmacro()

    add_example(triangle     win32/triangle_win32.cpp     triangle_render.cpp)
    add_example(cube         win32/cube_win32.cpp         cube_render.cpp)
    add_example(instanced    win32/instanced_win32.cpp    instanced_render.cpp)
    add_example(postprocess  win32/postprocess_win32.cpp  postprocess_render.cpp)
    add_example(floor        win32/floor_win32.cpp        floor_render.cpp)
endif()
