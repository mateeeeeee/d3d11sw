add_subdirectory(googletest)

add_library(bc_decomp STATIC bc7enc/bc7decomp.cpp bc7enc/rgbcx_impl.cpp)
target_include_directories(bc_decomp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(bc_decomp PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(tessellator STATIC tessellator.cpp)
target_include_directories(tessellator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(tessellator PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(tessellator PRIVATE -Wno-everything)
