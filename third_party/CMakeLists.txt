add_subdirectory(googletest)

add_library(bc_decomp STATIC bc7enc/bc7decomp.cpp bc7enc/rgbcx_impl.cpp)
target_include_directories(bc_decomp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(bc_decomp PROPERTIES POSITION_INDEPENDENT_CODE ON)
