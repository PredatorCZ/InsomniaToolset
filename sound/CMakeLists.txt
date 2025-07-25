cmake_minimum_required(VERSION 3.12)

project(SoundExtract)

build_target(
  NAME
  extract_sound
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  extract_sound.cpp
  LINKS
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Insomnia Engine extract resident sound"
  START_YEAR
  2017)
