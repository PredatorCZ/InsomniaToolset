if(BUILD_STATIC_LIBS)
  set(BUILD_STATIC_OBJECTS ON)
endif()

if(BUILD_SHARED_LIBS)
  set(BUILD_SHARED_OBJECTS ON)
endif()

if(BUILD_STATIC_OBJECTS)
  build_target(
    NAME
    insomnia-objects
    TYPE
    OBJECT
    SOURCES
    serialize
    reflected
    NO_VERINFO
    NO_PROJECT_H)

  target_include_directories(insomnia-objects PUBLIC ../include)

  set_precore_sources(
    insomnia-objects
    no_rtti
    uni_format)
endif()

if(BUILD_SHARED_OBJECTS)
  build_target(
    NAME
    insomnia-shared
    TYPE
    OBJECT
    SOURCES
    serialize
    reflected
    PROPERTIES
    POSITION_INDEPENDENT_CODE
    ON
    NO_VERINFO
    NO_PROJECT_H)

  target_include_directories(insomnia-shared PUBLIC ../include)

  if(PC_SHARED_LIB)
    target_link_libraries(insomnia-shared precore)
  else()
    set_precore_sources(
      insomnia-shared
      no_rtti
      uni_format)
  endif()
endif()

if(BUILD_STATIC_LIBS)
  build_target(
    NAME
    insomnia-static
    PROPERTIES
    OUTPUT_NAME
    insomnia
    TYPE
    STATIC
    LINKS
    insomnia-objects
    NO_VERINFO
    NO_PROJECT_H)
  install(TARGETS insomnia-static LIBRARY DESTINATION lib)
endif()

if(BUILD_SHARED_LIBS)
  target_compile_definitions(insomnia-shared PRIVATE IS_EXPORT)

  build_target(
    NAME
    insomnia
    TYPE
    SHARED
    LINKS
    insomnia-shared
    START_YEAR
    2017
    AUTHOR
    "Lukas Cone"
    NO_PROJECT_H)

  target_compile_definitions(insomnia INTERFACE IS_IMPORT)
  if(NOT MSVC)
    target_link_libraries(insomnia -Wl,-z,defs)
  endif()
  install(
    TARGETS insomnia
    LIBRARY NAMELINK_SKIP DESTINATION lib
    RUNTIME DESTINATION bin)
endif()
