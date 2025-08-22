cmake_minimum_required(VERSION 3.12)

project(LevelmainToGLTF)

build_target(
  NAME
  levelmain_to_gltf
  TYPE
  ESMODULE
  VERSION
  2
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

project(GatherData)

build_target(
  NAME
  gather_rfom
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  gather.cpp
  ${TPD_PATH}/mikktspace/mikktspace.c
  INCLUDES
  ${TPD_PATH}/mikktspace
  LINKS
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Gather RFOM data"
  START_YEAR
  2025)
