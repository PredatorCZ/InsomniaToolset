cmake_minimum_required(VERSION 3.12)

project(EffectExtract VERSION 1.0)

build_target(
  NAME
  extract_effect
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  extract_effect.cpp
  INCLUDES
  LINKS
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Insomnia Engine effect extract"
  START_YEAR
  2023)
