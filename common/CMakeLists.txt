project(InsomniaLib VERSION 1)

build_target(
  NAME
  insomnia
  TYPE
  SHARED
  SOURCES
  src/serialize.cpp
  src/reflected.cpp
  LINKS
  precore
  NO_VERINFO
  NO_PROJECT_H)

target_compile_definitions(
  insomnia
  PRIVATE IS_EXPORT REF_EXPORT
  INTERFACE IS_IMPORT)

target_include_directories(insomnia PUBLIC include)

install(
  TARGETS insomnia
  LIBRARY NAMELINK_SKIP DESTINATION $<IF:$<BOOL:${MINGW}>,bin,lib>
  RUNTIME DESTINATION bin)
