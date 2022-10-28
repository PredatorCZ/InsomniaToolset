cmake_minimum_required(VERSION 3.3)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "X86 architecture is not supported!")
endif()

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra)
set(BUILD_SHARED_LIBS ON)
set(NO_OBJECTS TRUE)

set(TPD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)
add_subdirectory(${TPD_PATH}/precore)
include(targetex)
add_subdirectory(common)

project(InsomniaToolset VERSION 1.0)
add_subdirectory(${TPD_PATH}/precore/spike
                 ${CMAKE_CURRENT_BINARY_DIR}/spike)
# add_subdirectory(${TPD_PATH}/gltf)
set_target_properties(spike PROPERTIES OUTPUT_NAME insomnia_toolset)

add_spike_subdir(extract)
