cmake_minimum_required(VERSION 3.12)

project(LevelmainToGLTF)

build_target(
  NAME
  levelmain_to_gltf
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  extract.cpp
  LINKS
  gltf-interface
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Insomnia Engine levelmain extract"
  START_YEAR
  2024)
