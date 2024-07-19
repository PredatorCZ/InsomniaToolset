project(InsomniaLib VERSION 1)

add_library(insomnia-interface INTERFACE)
target_include_directories(insomnia-interface INTERFACE include)
target_link_libraries(insomnia-interface INTERFACE spike-interface pugixml-interface)
set(CORE_SOURCE_FILES src/serialize.cpp;src/reflected.cpp)

if(NOT NO_OBJECTS)
  add_library(insomnia-objects OBJECT ${CORE_SOURCE_FILES})
  target_link_libraries(insomnia-objects PUBLIC insomnia-interface)
  set_target_properties(insomnia-objects PROPERTIES POSITION_INDEPENDENT_CODE
                                                    ${OBJECTS_PID})
  target_compile_options(insomnia-objects PRIVATE -fvisibility=hidden)

  target_expose_defs(insomnia IS_EXPORT IS_IMPORT)
endif()

if(BUILD_SHARED_LIBS)
  add_library(insomnia SHARED ${CORE_SOURCE_FILES})
  target_link_libraries(insomnia insomnia-interface spike pugixml)
  target_compile_definitions(
    insomnia
    INTERFACE IS_IMPORT
    PRIVATE IS_EXPORT)

  if(WIN32 OR MINGW)
    set_target_properties(
      insomnia PROPERTIES SUFFIX ${PROJECT_VERSION_MAJOR}.dll PREFIX "")
  else()
    set_target_properties(
      insomnia PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION
                                                     ${PROJECT_VERSION_MAJOR})
  endif()

  install(
    TARGETS insomnia
    LIBRARY NAMELINK_SKIP DESTINATION $<IF:$<BOOL:${MINGW}>,bin,lib>
    RUNTIME DESTINATION bin)
endif()
