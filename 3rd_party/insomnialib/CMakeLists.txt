cmake_minimum_required(VERSION 3.12)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "X86 architecture is not supported!")
endif()

project(InsomniaLib VERSION 1)

# ~~~
# Valid targets:
#  - insomnia-static: static library target (BUILD_STATIC_LIBS == ON OR BUILD_STATIC_OBJECTS == ON)
#  - insomnia-objects: static objects target (BUILD_STATIC_OBJECTS == ON)
#  - insomnia-shared: PIC objects target (BUILD_SHARED_OBJECTS == ON OR BUILD_SHARED_LIBS == ON)
#  - insomnia: shared library target (BUILD_SHARED_LIBS == ON), insomnia-shared target will have exported symbols
#
# Definitions:
#  - IS_EXPORT: use to export dynamic symbols from InsomniaLib target (BUILD_SHARED_LIBS == ON)
#  - IS_IMPORT: use to import dynamic symbols from shared library (API only)
# ~~~

option(ODR_TEST "Enable ODR testing." OFF)
option(BUILD_STATIC_OBJECTS "Builds insomnia-objects target." ON)
option(BUILD_SHARED_OBJECTS "Builds insomnia-shared target." OFF)
option(BUILD_STATIC_LIBS "Builds insomnia-static target." OFF)
option(BUILD_SHARED_LIBS "Builds insomnia target." OFF)

set(CMAKE_CXX_STANDARD 17)
# add_subdirectory(3rd_party/PreCore) 
include(targetex) 
include(chartype)

add_subdirectory(src)
