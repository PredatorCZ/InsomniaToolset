cmake_minimum_required(VERSION 3.12)

project(AssetExtract)

build_target(
  NAME
  extract_assets
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  extract_v2.cpp
  gltf.cpp
  LINKS
  gltf-interface
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Insomnia Engine asset extract"
  START_YEAR
  2017)

project(Region2GLTF)

build_target(
  NAME
  region_to_gltf
  TYPE
  ESMODULE
  VERSION
  1
  SOURCES
  region_to_gltf.cpp
  gltf.cpp
  LINKS
  gltf-interface
  insomnia-interface
  AUTHOR
  "Lukas Cone"
  DESCR
  "Insomnia Engine convert region to GLTF"
  START_YEAR
  2025)
