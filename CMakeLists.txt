cmake_minimum_required(VERSION 3.3)

set(CMAKE_CXX_STANDARD 17)
project(InsomniaToolset VERSION 1.0)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

set(TPD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)
set(BUILD_SHARED_LIBS
    ON
    CACHE BOOL "")
set(BUILD_STATIC_OBJECTS
    OFF
    CACHE BOOL "")
set(PUGI_SHARED_LIBS
    ON
    CACHE BOOL "")
set(PC_SHARED_LIB
    ON
    CACHE BOOL "")

add_subdirectory(${TPD_PATH}/precore)
add_subdirectory(${TPD_PATH}/insomnialib)
add_subdirectory(${TPD_PATH}/precore/spike
                 ${CMAKE_CURRENT_BINARY_DIR}/spike)
# add_subdirectory(${TPD_PATH}/gltf)
set_target_properties(spike PROPERTIES OUTPUT_NAME insomnia_toolset)

include(targetex)
include(chartype)

add_spike_subdir(extract)
