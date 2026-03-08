project(TextureExtract)

build_target(
  NAME
  extract_textures
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  extract_textures.cpp
  LINKS
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Extract textures from Luna tp/tph pair"
  START_YEAR
  2026)
