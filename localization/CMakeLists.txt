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
  "Convert RFOM localization data to JSON"
  START_YEAR
  2026)

project(JSON2Localization)

build_target(
  NAME
  json_to_loc
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  json_to_loc.cpp
  LINKS
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Convert JSON to localization data"
  START_YEAR
  2026)

project(LocalizationV22JSON)

build_target(
  NAME
  locv2_to_json
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  locv2_to_json.cpp
  LINKS
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Convert V2 localization data to JSON"
  START_YEAR
  2026)
