project(Localization2JSON)

build_target(
  NAME
  loc_to_json
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  loc_to_json.cpp
  LINKS
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Convert localization data to JSON"
  START_YEAR
  2026)
