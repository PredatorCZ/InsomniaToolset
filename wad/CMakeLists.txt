cmake_minimum_required(VERSION 3.12)

project(GameWAD)

build_target(
  NAME
  game_wad
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  game_wad.cpp
  LINKS
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "R:FOM game.wad + game.wad.lst"
  START_YEAR
  2026)

project(MpegWAD)

build_target(
  NAME
  mpeg_wad
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  mpeg_wad.cpp
  LINKS
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "R:FOM mpeg.wad + mpeg.wad.lst"
  START_YEAR
  2026)

project(WADPack)

build_target(
  NAME
  wadpack_game
  TYPE
  APP
  VERSION
  1
  SOURCES
  wadpack_game.cpp
  LINKS
  spike-objects
  pugixml-objects
  AUTHOR
  "Lukas Cone"
  DESCR
  "Generate R:FOM Game WAD"
  START_YEAR
  2026)

build_target(
  NAME
  wadpack_mpeg
  TYPE
  APP
  VERSION
  1
  SOURCES
  wadpack_mpeg.cpp
  LINKS
  spike-objects
  pugixml-objects
  AUTHOR
  "Lukas Cone"
  DESCR
  "Generate R:FOM Mpeg WAD"
  START_YEAR
  2026)
