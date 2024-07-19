cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
project(InsomniaToolset)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-ignored-qualifiers")

option(CLI "" ON)
option(GLTF "" ON)
set(EXPOSE_SYMBOLS spike;pugixml;gltf;insomnia)

set(TPD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)

add_subdirectory(${TPD_PATH}/spike)
include(version)
toolset_version()

set_target_properties(spike_cli PROPERTIES OUTPUT_NAME insomnia_toolset)

add_subdirectory(common)
target_link_libraries(spike_cli insomnia-objects)
add_spike_subdir(extract)
add_spike_subdir(effect)

set(TPD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/fragmented/3rd_party)
add_spike_subdir(3rd_party/fragmented/psarc)
